#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

void parser_init(Parser *parser, Lexer *lexer) {
    parser->lexer = lexer;
    parser->current = lexer_next_token(lexer);
    parser->func_count = 0;
    parser->loop_depth = 0;
    parser->switch_depth = 0;
}

static FuncSig *parser_find_function(Parser *parser, const char *name) {
    for (int i = 0; i < parser->func_count; i++) {
        if (strcmp(parser->funcs[i].name, name) == 0) {
            return &parser->funcs[i];
        }
    }
    return NULL;
}

/*
 * Record a function's appearance (declaration or definition). If the name is
 * already in the table, enforce:
 *   - parameter counts must match
 *   - at most one definition per name
 * Otherwise add a new entry.
 */
static void parser_register_function(Parser *parser, const char *name,
                                     int param_count, int is_definition) {
    FuncSig *existing = parser_find_function(parser, name);
    if (existing) {
        if (existing->param_count != param_count) {
            fprintf(stderr,
                    "error: function '%s' parameter count mismatch (%d vs %d)\n",
                    name, existing->param_count, param_count);
            exit(1);
        }
        if (is_definition) {
            if (existing->has_definition) {
                fprintf(stderr,
                        "error: duplicate function definition '%s'\n",
                        name);
                exit(1);
            }
            existing->has_definition = 1;
        }
        return;
    }
    if (parser->func_count >= PARSER_MAX_FUNCTIONS) {
        fprintf(stderr, "error: too many functions (max %d)\n", PARSER_MAX_FUNCTIONS);
        exit(1);
    }
    strncpy(parser->funcs[parser->func_count].name, name, 255);
    parser->funcs[parser->func_count].name[255] = '\0';
    parser->funcs[parser->func_count].param_count = param_count;
    parser->funcs[parser->func_count].has_definition = is_definition;
    parser->func_count++;
}

static Token parser_expect(Parser *parser, TokenType type) {
    if (parser->current.type != type) {
        fprintf(stderr, "error: expected token type %d, got %d ('%s')\n",
                type, parser->current.type, parser->current.value);
        exit(1);
    }
    Token token = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    return token;
}

/*
 * <primary> ::= <int_literal> | "(" <expression> ")"
 */
static ASTNode *parse_expression(Parser *parser);

static ASTNode *parse_primary(Parser *parser) {
    if (parser->current.type == TOKEN_INT_LITERAL) {
        Token token = parser_expect(parser, TOKEN_INT_LITERAL);
        return ast_new(AST_INT_LITERAL, token.value);
    }
    if (parser->current.type == TOKEN_IDENTIFIER) {
        Token token = parser_expect(parser, TOKEN_IDENTIFIER);
        if (parser->current.type == TOKEN_LPAREN) {
            parser_expect(parser, TOKEN_LPAREN);
            ASTNode *call = ast_new(AST_FUNCTION_CALL, token.value);
            if (parser->current.type != TOKEN_RPAREN) {
                ast_add_child(call, parse_expression(parser));
                while (parser->current.type == TOKEN_COMMA) {
                    parser->current = lexer_next_token(parser->lexer);
                    ast_add_child(call, parse_expression(parser));
                }
            }
            parser_expect(parser, TOKEN_RPAREN);
            FuncSig *sig = parser_find_function(parser, token.value);
            if (!sig) {
                fprintf(stderr, "error: call to undefined function '%s'\n",
                        token.value);
                exit(1);
            }
            if (sig->param_count != call->child_count) {
                fprintf(stderr,
                        "error: function '%s' expects %d arguments, got %d\n",
                        token.value, sig->param_count, call->child_count);
                exit(1);
            }
            if (call->child_count > 6) {
                fprintf(stderr,
                        "error: function '%s' call has %d arguments; max supported is 6\n",
                        token.value, call->child_count);
                exit(1);
            }
            return call;
        }
        return ast_new(AST_VAR_REF, token.value);
    }
    if (parser->current.type == TOKEN_LPAREN) {
        parser_expect(parser, TOKEN_LPAREN);
        ASTNode *expr = parse_expression(parser);
        parser_expect(parser, TOKEN_RPAREN);
        return expr;
    }
    fprintf(stderr, "error: expected expression, got '%s'\n",
            parser->current.value);
    exit(1);
}

/*
 * <postfix_expression> ::= <primary_expression> { "++" | "--" }
 */
static ASTNode *parse_postfix(Parser *parser) {
    ASTNode *expr = parse_primary(parser);
    while (parser->current.type == TOKEN_INCREMENT ||
           parser->current.type == TOKEN_DECREMENT) {
        if (expr->type != AST_VAR_REF) {
            fprintf(stderr, "error: postfix %s requires an identifier\n",
                    parser->current.value);
            exit(1);
        }
        Token op = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *node = ast_new(AST_POSTFIX_INC_DEC, op.value);
        ast_add_child(node, expr);
        expr = node;
    }
    return expr;
}

/*
 * <unary_expr> ::= [ "+" | "-" | "!" ] <unary_expr>
 *                | "++" <identifier>
 *                | "--" <identifier>
 *                | <postfix_expression>
 */
static ASTNode *parse_unary(Parser *parser) {
    if (parser->current.type == TOKEN_INCREMENT ||
        parser->current.type == TOKEN_DECREMENT) {
        Token op = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *operand = parse_unary(parser);
        if (operand->type != AST_VAR_REF) {
            fprintf(stderr, "error: prefix %s requires an identifier\n", op.value);
            exit(1);
        }
        ASTNode *node = ast_new(AST_PREFIX_INC_DEC, op.value);
        ast_add_child(node, operand);
        return node;
    }
    if (parser->current.type == TOKEN_PLUS ||
        parser->current.type == TOKEN_MINUS ||
        parser->current.type == TOKEN_BANG) {
        Token op = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *operand = parse_unary(parser);
        ASTNode *unary = ast_new(AST_UNARY_OP, op.value);
        ast_add_child(unary, operand);
        return unary;
    }
    return parse_postfix(parser);
}

/*
 * <term> ::= <unary_expr> { ("*" | "/") <unary_expr> }*
 */
static ASTNode *parse_term(Parser *parser) {
    ASTNode *left = parse_unary(parser);
    while (parser->current.type == TOKEN_STAR ||
           parser->current.type == TOKEN_SLASH) {
        Token op = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *right = parse_unary(parser);
        ASTNode *binop = ast_new(AST_BINARY_OP, op.value);
        ast_add_child(binop, left);
        ast_add_child(binop, right);
        left = binop;
    }
    return left;
}

/*
 * <additive_expr> ::= <term> { ("+" | "-") <term> }*
 */
static ASTNode *parse_additive(Parser *parser) {
    ASTNode *left = parse_term(parser);
    while (parser->current.type == TOKEN_PLUS ||
           parser->current.type == TOKEN_MINUS) {
        Token op = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *right = parse_term(parser);
        ASTNode *binop = ast_new(AST_BINARY_OP, op.value);
        ast_add_child(binop, left);
        ast_add_child(binop, right);
        left = binop;
    }
    return left;
}

/*
 * <relational_expr> ::= <additive_expr> { ("<" | "<=" | ">" | ">=") <additive_expr> }*
 */
static ASTNode *parse_relational(Parser *parser) {
    ASTNode *left = parse_additive(parser);
    while (parser->current.type == TOKEN_LT ||
           parser->current.type == TOKEN_LE ||
           parser->current.type == TOKEN_GT ||
           parser->current.type == TOKEN_GE) {
        Token op = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *right = parse_additive(parser);
        ASTNode *binop = ast_new(AST_BINARY_OP, op.value);
        ast_add_child(binop, left);
        ast_add_child(binop, right);
        left = binop;
    }
    return left;
}

/*
 * <equality_expr> ::= <relational_expr> { ("==" | "!=") <relational_expr> }*
 */
static ASTNode *parse_equality(Parser *parser) {
    ASTNode *left = parse_relational(parser);
    while (parser->current.type == TOKEN_EQ ||
           parser->current.type == TOKEN_NE) {
        Token op = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *right = parse_relational(parser);
        ASTNode *binop = ast_new(AST_BINARY_OP, op.value);
        ast_add_child(binop, left);
        ast_add_child(binop, right);
        left = binop;
    }
    return left;
}

/*
 * <logical_and_expression> ::= <equality_expression> { "&&" <equality_expression> }
 */
static ASTNode *parse_logical_and(Parser *parser) {
    ASTNode *left = parse_equality(parser);
    while (parser->current.type == TOKEN_AND_AND) {
        Token op = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *right = parse_equality(parser);
        ASTNode *binop = ast_new(AST_BINARY_OP, op.value);
        ast_add_child(binop, left);
        ast_add_child(binop, right);
        left = binop;
    }
    return left;
}

/*
 * <logical_or_expression> ::= <logical_and_expression> { "||" <logical_and_expression> }
 */
static ASTNode *parse_logical_or(Parser *parser) {
    ASTNode *left = parse_logical_and(parser);
    while (parser->current.type == TOKEN_OR_OR) {
        Token op = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *right = parse_logical_and(parser);
        ASTNode *binop = ast_new(AST_BINARY_OP, op.value);
        ast_add_child(binop, left);
        ast_add_child(binop, right);
        left = binop;
    }
    return left;
}

/*
 * <expression> ::= <assignment_expression>
 *
 * <assignment_expression> ::= <identifier> "=" <assignment_expression>
 *                            | <identifier> "+=" <assignment_expression>
 *                            | <identifier> "-=" <assignment_expression>
 *                            | <logical_or_expression>
 */
static ASTNode *parse_expression(Parser *parser) {
    if (parser->current.type == TOKEN_IDENTIFIER) {
        int saved_pos = parser->lexer->pos;
        Token saved_token = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        if (parser->current.type == TOKEN_ASSIGN ||
            parser->current.type == TOKEN_PLUS_ASSIGN ||
            parser->current.type == TOKEN_MINUS_ASSIGN) {
            Token op = parser->current;
            parser->current = lexer_next_token(parser->lexer);
            ASTNode *rhs = parse_expression(parser);
            ASTNode *assign = ast_new(AST_ASSIGNMENT, saved_token.value);
            if (op.type == TOKEN_PLUS_ASSIGN || op.type == TOKEN_MINUS_ASSIGN) {
                /* a += b  =>  a = a + b */
                ASTNode *var_ref = ast_new(AST_VAR_REF, saved_token.value);
                const char *bin_op = (op.type == TOKEN_PLUS_ASSIGN) ? "+" : "-";
                ASTNode *binop = ast_new(AST_BINARY_OP, bin_op);
                ast_add_child(binop, var_ref);
                ast_add_child(binop, rhs);
                ast_add_child(assign, binop);
            } else {
                ast_add_child(assign, rhs);
            }
            return assign;
        }
        /* Not assignment — restore lexer state */
        parser->lexer->pos = saved_pos;
        parser->current = saved_token;
    }
    return parse_logical_or(parser);
}

/*
 * <block> ::= "{" { <statement> } "}"
 */
static ASTNode *parse_statement(Parser *parser);

static ASTNode *parse_block(Parser *parser) {
    parser_expect(parser, TOKEN_LBRACE);
    ASTNode *block = ast_new(AST_BLOCK, NULL);
    while (parser->current.type != TOKEN_RBRACE) {
        ast_add_child(block, parse_statement(parser));
    }
    parser_expect(parser, TOKEN_RBRACE);
    return block;
}

/*
 * <if_statement> ::= "if" "(" <expression> ")" <statement> [ "else" <statement> ]
 */
static ASTNode *parse_if_statement(Parser *parser) {
    parser_expect(parser, TOKEN_IF);
    parser_expect(parser, TOKEN_LPAREN);
    ASTNode *condition = parse_expression(parser);
    parser_expect(parser, TOKEN_RPAREN);
    ASTNode *then_stmt = parse_statement(parser);

    ASTNode *if_node = ast_new(AST_IF_STATEMENT, NULL);
    ast_add_child(if_node, condition);
    ast_add_child(if_node, then_stmt);

    if (parser->current.type == TOKEN_ELSE) {
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *else_stmt = parse_statement(parser);
        ast_add_child(if_node, else_stmt);
    }

    return if_node;
}

/*
 * <while_statement> ::= "while" "(" <expression> ")" <statement>
 */
static ASTNode *parse_while_statement(Parser *parser) {
    parser_expect(parser, TOKEN_WHILE);
    parser_expect(parser, TOKEN_LPAREN);
    ASTNode *condition = parse_expression(parser);
    parser_expect(parser, TOKEN_RPAREN);
    parser->loop_depth++;
    ASTNode *body = parse_statement(parser);
    parser->loop_depth--;

    ASTNode *while_node = ast_new(AST_WHILE_STATEMENT, NULL);
    ast_add_child(while_node, condition);
    ast_add_child(while_node, body);

    return while_node;
}

/*
 * <do_while_statement> ::= "do" <statement> "while" "(" <expression> ")" ";"
 */
static ASTNode *parse_do_while_statement(Parser *parser) {
    parser_expect(parser, TOKEN_DO);
    parser->loop_depth++;
    ASTNode *body = parse_statement(parser);
    parser->loop_depth--;
    parser_expect(parser, TOKEN_WHILE);
    parser_expect(parser, TOKEN_LPAREN);
    ASTNode *condition = parse_expression(parser);
    parser_expect(parser, TOKEN_RPAREN);
    parser_expect(parser, TOKEN_SEMICOLON);

    ASTNode *do_while_node = ast_new(AST_DO_WHILE_STATEMENT, NULL);
    ast_add_child(do_while_node, body);
    ast_add_child(do_while_node, condition);

    return do_while_node;
}

/*
 * <for_statement> ::= "for" "(" [<expression>] ";" [<expression>] ";" [<expression>] ")" <statement>
 *
 * Children layout: [init, condition, update, body]
 * Any of init/condition/update may be NULL when omitted.
 */
static ASTNode *parse_for_statement(Parser *parser) {
    parser_expect(parser, TOKEN_FOR);
    parser_expect(parser, TOKEN_LPAREN);

    ASTNode *for_node = ast_new(AST_FOR_STATEMENT, NULL);

    /* init */
    ASTNode *init = NULL;
    if (parser->current.type != TOKEN_SEMICOLON) {
        init = parse_expression(parser);
    }
    parser_expect(parser, TOKEN_SEMICOLON);

    /* condition */
    ASTNode *condition = NULL;
    if (parser->current.type != TOKEN_SEMICOLON) {
        condition = parse_expression(parser);
    }
    parser_expect(parser, TOKEN_SEMICOLON);

    /* update */
    ASTNode *update = NULL;
    if (parser->current.type != TOKEN_RPAREN) {
        update = parse_expression(parser);
    }
    parser_expect(parser, TOKEN_RPAREN);

    parser->loop_depth++;
    ASTNode *body = parse_statement(parser);
    parser->loop_depth--;

    /* Store as children — NULLs are stored directly */
    for_node->children[0] = init;
    for_node->children[1] = condition;
    for_node->children[2] = update;
    for_node->children[3] = body;
    for_node->child_count = 4;

    return for_node;
}

/*
 * <switch_statement> ::= "switch" "(" <expression> ")" <switch_body>
 * <switch_body>      ::= "{" <switch_section> { <switch_section> } "}"
 * <switch_section>   ::= <case_section> | <default_section>
 * <case_section>     ::= "case" <integer_literal> ":" { <statement> }
 * <default_section>  ::= "default" ":" { <statement> }
 *
 * Stage 10-03-02: the switch body is a sequence of case/default
 * sections. At most one default is allowed. Case values must be
 * integer literals. Statements within a section are consumed until
 * the next `case`, `default`, or closing `}`.
 */
static ASTNode *parse_switch_statement(Parser *parser) {
    parser_expect(parser, TOKEN_SWITCH);
    parser_expect(parser, TOKEN_LPAREN);
    ASTNode *expr = parse_expression(parser);
    parser_expect(parser, TOKEN_RPAREN);
    parser_expect(parser, TOKEN_LBRACE);

    ASTNode *switch_node = ast_new(AST_SWITCH_STATEMENT, NULL);
    ast_add_child(switch_node, expr);

    parser->switch_depth++;
    int default_seen = 0;
    while (parser->current.type != TOKEN_RBRACE) {
        if (parser->current.type == TOKEN_CASE) {
            parser_expect(parser, TOKEN_CASE);
            Token value = parser_expect(parser, TOKEN_INT_LITERAL);
            parser_expect(parser, TOKEN_COLON);
            ASTNode *case_section = ast_new(AST_CASE_SECTION, NULL);
            ast_add_child(case_section, ast_new(AST_INT_LITERAL, value.value));
            while (parser->current.type != TOKEN_CASE &&
                   parser->current.type != TOKEN_DEFAULT &&
                   parser->current.type != TOKEN_RBRACE) {
                ast_add_child(case_section, parse_statement(parser));
            }
            ast_add_child(switch_node, case_section);
        } else if (parser->current.type == TOKEN_DEFAULT) {
            if (default_seen) {
                fprintf(stderr, "error: duplicate default section in switch\n");
                exit(1);
            }
            default_seen = 1;
            parser_expect(parser, TOKEN_DEFAULT);
            parser_expect(parser, TOKEN_COLON);
            ASTNode *default_section = ast_new(AST_DEFAULT_SECTION, NULL);
            while (parser->current.type != TOKEN_CASE &&
                   parser->current.type != TOKEN_DEFAULT &&
                   parser->current.type != TOKEN_RBRACE) {
                ast_add_child(default_section, parse_statement(parser));
            }
            ast_add_child(switch_node, default_section);
        } else {
            parser_expect(parser, TOKEN_CASE); /* reports "expected token type" */
        }
    }
    parser->switch_depth--;

    parser_expect(parser, TOKEN_RBRACE);

    return switch_node;
}

/*
 * <statement> ::= <declaration>
 *               | <assignment_statement>
 *               | <return_statement>
 *               | <if_statement>
 *               | <while_statement>
 *               | <do_while_statement>
 *               | <for_statement>
 *               | <switch_statement>
 *               | <block>
 *               | <expression_stmt>
 */
static ASTNode *parse_statement(Parser *parser) {
    /* declaration: "int" <identifier> [ "=" <expression> ] ";" */
    if (parser->current.type == TOKEN_INT) {
        parser->current = lexer_next_token(parser->lexer);
        Token name = parser_expect(parser, TOKEN_IDENTIFIER);
        ASTNode *decl = ast_new(AST_DECLARATION, name.value);
        if (parser->current.type == TOKEN_ASSIGN) {
            parser->current = lexer_next_token(parser->lexer);
            ASTNode *init = parse_expression(parser);
            ast_add_child(decl, init);
        }
        parser_expect(parser, TOKEN_SEMICOLON);
        return decl;
    }
    if (parser->current.type == TOKEN_RETURN) {
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *expr = parse_expression(parser);
        parser_expect(parser, TOKEN_SEMICOLON);
        ASTNode *stmt = ast_new(AST_RETURN_STATEMENT, NULL);
        ast_add_child(stmt, expr);
        return stmt;
    }
    if (parser->current.type == TOKEN_IF) {
        return parse_if_statement(parser);
    }
    if (parser->current.type == TOKEN_WHILE) {
        return parse_while_statement(parser);
    }
    if (parser->current.type == TOKEN_DO) {
        return parse_do_while_statement(parser);
    }
    if (parser->current.type == TOKEN_FOR) {
        return parse_for_statement(parser);
    }
    if (parser->current.type == TOKEN_SWITCH) {
        return parse_switch_statement(parser);
    }
    if (parser->current.type == TOKEN_LBRACE) {
        return parse_block(parser);
    }
    if (parser->current.type == TOKEN_BREAK) {
        if (parser->loop_depth == 0 && parser->switch_depth == 0) {
            fprintf(stderr, "error: break outside of loop or switch\n");
            exit(1);
        }
        parser->current = lexer_next_token(parser->lexer);
        parser_expect(parser, TOKEN_SEMICOLON);
        return ast_new(AST_BREAK_STATEMENT, NULL);
    }
    if (parser->current.type == TOKEN_CONTINUE) {
        if (parser->loop_depth == 0) {
            fprintf(stderr, "error: continue outside of loop\n");
            exit(1);
        }
        parser->current = lexer_next_token(parser->lexer);
        parser_expect(parser, TOKEN_SEMICOLON);
        return ast_new(AST_CONTINUE_STATEMENT, NULL);
    }
    /* expression_stmt (includes assignments, since assignment is now an expression) */
    ASTNode *expr = parse_expression(parser);
    parser_expect(parser, TOKEN_SEMICOLON);
    ASTNode *stmt = ast_new(AST_EXPRESSION_STMT, NULL);
    ast_add_child(stmt, expr);
    return stmt;
}

/*
 * <parameter_declaration> ::= "int" <identifier>
 */
static ASTNode *parse_parameter_declaration(Parser *parser) {
    parser_expect(parser, TOKEN_INT);
    Token name = parser_expect(parser, TOKEN_IDENTIFIER);
    return ast_new(AST_PARAM, name.value);
}

/*
 * <parameter_list> ::= <parameter_declaration> { "," <parameter_declaration> }
 *
 * Parses the parameter list and appends each AST_PARAM node as a child of
 * `func`. Enforces unique parameter names within the list.
 */
static void parse_parameter_list(Parser *parser, ASTNode *func) {
    ASTNode *param = parse_parameter_declaration(parser);
    ast_add_child(func, param);
    while (parser->current.type == TOKEN_COMMA) {
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *next = parse_parameter_declaration(parser);
        for (int i = 0; i < func->child_count; i++) {
            if (func->children[i]->type == AST_PARAM &&
                strcmp(func->children[i]->value, next->value) == 0) {
                fprintf(stderr,
                        "error: duplicate parameter '%s' in function '%s'\n",
                        next->value, func->value);
                exit(1);
            }
        }
        ast_add_child(func, next);
    }
}

/*
 * <function> ::= "int" <identifier> "(" [ <parameter_list> ] ")" ( <block> | ";" )
 *
 * AST layout for a definition: zero or more AST_PARAM children followed by
 * the AST_BLOCK body. A pure declaration has only the AST_PARAM children
 * (no AST_BLOCK).
 */
static ASTNode *parse_function_decl(Parser *parser) {
    parser_expect(parser, TOKEN_INT);
    Token name = parser_expect(parser, TOKEN_IDENTIFIER);
    ASTNode *func = ast_new(AST_FUNCTION_DECL, name.value);

    parser_expect(parser, TOKEN_LPAREN);
    if (parser->current.type != TOKEN_RPAREN) {
        parse_parameter_list(parser, func);
    }
    parser_expect(parser, TOKEN_RPAREN);

    int param_count = func->child_count;
    int is_definition = (parser->current.type == TOKEN_LBRACE);

    /* Register before parsing the body so self-calls resolve. The helper
     * also enforces param-count consistency and rejects duplicate
     * definitions. */
    parser_register_function(parser, name.value, param_count, is_definition);

    if (is_definition) {
        ASTNode *body = parse_block(parser);
        ast_add_child(func, body);
    } else {
        parser_expect(parser, TOKEN_SEMICOLON);
    }
    return func;
}

/*
 * <external_declaration> ::= <function>
 */
static ASTNode *parse_external_declaration(Parser *parser) {
    return parse_function_decl(parser);
}

/*
 * <translation_unit> ::= <external_declaration> { <external_declaration> }
 *
 * Duplicate-definition and signature-consistency rules are enforced in
 * parser_register_function; multiple declarations of the same function
 * are permitted.
 */
ASTNode *parse_translation_unit(Parser *parser) {
    ASTNode *unit = ast_new(AST_TRANSLATION_UNIT, NULL);
    do {
        ASTNode *ext_decl = parse_external_declaration(parser);
        ast_add_child(unit, ext_decl);
    } while (parser->current.type != TOKEN_EOF);
    parser_expect(parser, TOKEN_EOF);
    return unit;
}
