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
 * Otherwise add a new entry. The full typed signature (return type and
 * ordered parameter types) is captured from the first occurrence — per
 * spec, signature compatibility between declarations and definitions
 * beyond arity is not checked in this stage.
 */
static void parser_register_function(Parser *parser, const char *name,
                                     int param_count, int is_definition,
                                     TypeKind return_type,
                                     const TypeKind *param_types) {
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
    if (param_count > FUNC_MAX_PARAMS) {
        fprintf(stderr,
                "error: function '%s' has %d parameters; max supported is %d\n",
                name, param_count, FUNC_MAX_PARAMS);
        exit(1);
    }
    FuncSig *sig = &parser->funcs[parser->func_count];
    strncpy(sig->name, name, 255);
    sig->name[255] = '\0';
    sig->param_count = param_count;
    sig->has_definition = is_definition;
    sig->return_type = return_type;
    for (int i = 0; i < param_count; i++) {
        sig->param_types[i] = param_types[i];
    }
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
        ASTNode *node = ast_new(AST_INT_LITERAL, token.value);
        node->decl_type = token.literal_type;
        return node;
    }
    /* Stage 14-02: a string literal is a primary expression whose
     * logical type is char[N+1], where N is the literal's byte
     * length and the trailing slot holds the implicit NUL. The
     * payload bytes ride on node->value (copied below) and the
     * length is preserved on full_type->length.
     *
     * Stage 14-05: the decoded payload may contain embedded NUL
     * bytes (from `\0`), so the value is copied with memcpy bounded
     * by token.length rather than via ast_new's strncpy, and the
     * count is also stashed on byte_length for downstream consumers
     * (notably codegen, where full_type is rewritten to `char *`
     * during the array-to-pointer decay and the length on full_type
     * is no longer reachable). */
    if (parser->current.type == TOKEN_STRING_LITERAL) {
        Token token = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *node = ast_new(AST_STRING_LITERAL, NULL);
        memcpy(node->value, token.value, token.length);
        node->value[token.length] = '\0';
        node->byte_length = token.length;
        node->decl_type = TYPE_ARRAY;
        node->full_type = type_array(type_char(), token.length + 1);
        return node;
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
            /* The call expression is typed with the callee's declared
             * return type so downstream type rules (promotion, common
             * arithmetic type, etc.) see it. */
            call->decl_type = sig->return_type;
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
 * <postfix_expression> ::= <primary_expression>
 *                          { "[" <expression> "]" | "++" | "--" }
 *
 * Stage 13-02 added the subscript suffix; Stage 13-05 formalizes the
 * pointer-base case so the same suffix applies whether the identifier
 * names an array or a pointer. The base must still be an identifier in
 * this stage. The wrapped node is AST_ARRAY_INDEX with children
 * [base, index]. Type-driven dispatch (array vs pointer) and the
 * integer-index check happen in codegen.
 */
static ASTNode *parse_postfix(Parser *parser) {
    ASTNode *expr = parse_primary(parser);
    while (parser->current.type == TOKEN_INCREMENT ||
           parser->current.type == TOKEN_DECREMENT ||
           parser->current.type == TOKEN_LBRACKET) {
        if (parser->current.type == TOKEN_LBRACKET) {
            if (expr->type != AST_VAR_REF) {
                fprintf(stderr, "error: subscript base must be an identifier\n");
                exit(1);
            }
            parser->current = lexer_next_token(parser->lexer);
            ASTNode *index = parse_expression(parser);
            parser_expect(parser, TOKEN_RBRACKET);
            ASTNode *node = ast_new(AST_ARRAY_INDEX, NULL);
            ast_add_child(node, expr);
            ast_add_child(node, index);
            expr = node;
            continue;
        }
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
 * <unary_expr> ::= ( "+" | "-" | "!" | "++" | "--" | "*" | "&" ) <unary_expr>
 *                | <postfix_expression>
 *
 * Stage 12-02 adds unary "&" (address-of) and unary "*" (dereference).
 * Address-of requires an lvalue; the only lvalue this stage supports
 * is a plain variable reference, so the operand must be AST_VAR_REF.
 * Unary "*" only fires when "*" begins a unary expression — binary
 * "*" continues to work because parse_term consumes the left operand
 * before looking for "*".
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
    if (parser->current.type == TOKEN_AMPERSAND) {
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *operand = parse_unary(parser);
        /* Stage 13-04: address-of also accepts an array subscript so
         * `&a[i]` produces a pointer to the i-th element. */
        if (operand->type != AST_VAR_REF &&
            operand->type != AST_ARRAY_INDEX) {
            fprintf(stderr, "error: address-of requires an lvalue\n");
            exit(1);
        }
        ASTNode *node = ast_new(AST_ADDR_OF, NULL);
        ast_add_child(node, operand);
        return node;
    }
    if (parser->current.type == TOKEN_STAR) {
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *operand = parse_unary(parser);
        ASTNode *node = ast_new(AST_DEREF, NULL);
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
 * <cast_expression> ::= <unary_expression>
 *                     | "(" <integer_type> ")" <cast_expression>
 *
 * When the current token is '(', save lexer state and peek past it to
 * decide between a cast and a parenthesized expression. If the next
 * token is an integer-type keyword, consume "(", <type>, ")" and
 * recurse to parse the operand cast expression. Otherwise restore
 * the saved state and fall through to parse_unary — parenthesized
 * expressions are then handled by parse_primary as before.
 */
static ASTNode *parse_cast(Parser *parser) {
    if (parser->current.type == TOKEN_LPAREN) {
        int saved_pos = parser->lexer->pos;
        Token saved_token = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        if (parser->current.type == TOKEN_CHAR ||
            parser->current.type == TOKEN_SHORT ||
            parser->current.type == TOKEN_INT ||
            parser->current.type == TOKEN_LONG) {
            TypeKind target_kind;
            switch (parser->current.type) {
            case TOKEN_CHAR:  target_kind = TYPE_CHAR;  break;
            case TOKEN_SHORT: target_kind = TYPE_SHORT; break;
            case TOKEN_LONG:  target_kind = TYPE_LONG;  break;
            default:          target_kind = TYPE_INT;   break;
            }
            parser->current = lexer_next_token(parser->lexer);
            parser_expect(parser, TOKEN_RPAREN);
            ASTNode *operand = parse_cast(parser);
            ASTNode *cast = ast_new(AST_CAST, NULL);
            cast->decl_type = target_kind;
            ast_add_child(cast, operand);
            return cast;
        }
        /* Not a cast — restore lexer state */
        parser->lexer->pos = saved_pos;
        parser->current = saved_token;
    }
    return parse_unary(parser);
}

/*
 * <term> ::= <cast_expression> { ("*" | "/") <cast_expression> }*
 */
static ASTNode *parse_term(Parser *parser) {
    ASTNode *left = parse_cast(parser);
    while (parser->current.type == TOKEN_STAR ||
           parser->current.type == TOKEN_SLASH) {
        Token op = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *right = parse_cast(parser);
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
 *                            | <unary_expression> "=" <assignment_expression>
 *                            | <logical_or_expression>
 *
 * Stage 12-03 adds the dereference-LHS form so `*p = value` parses.
 * The LHS is parsed as a logical-or expression and then validated to
 * be an lvalue (AST_DEREF or AST_VAR_REF). Non-lvalue LHS such as
 * `(x + 1) = 3` is rejected here.
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
    ASTNode *lhs = parse_logical_or(parser);
    if (parser->current.type == TOKEN_ASSIGN) {
        if (lhs->type != AST_DEREF && lhs->type != AST_VAR_REF &&
            lhs->type != AST_ARRAY_INDEX) {
            fprintf(stderr, "error: assignment target must be an lvalue\n");
            exit(1);
        }
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *rhs = parse_expression(parser);
        ASTNode *assign = ast_new(AST_ASSIGNMENT, NULL);
        ast_add_child(assign, lhs);
        ast_add_child(assign, rhs);
        return assign;
    }
    return lhs;
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
 * <switch_statement>   ::= "switch" "(" <expression> ")" <statement>
 * <labeled_statement>  ::= "case" <integer_literal> ":" <statement>
 *                        | "default" ":" <statement>
 *
 * Stage 10-03-03: the switch body is any statement. `case` and
 * `default` labels are parsed as labeled statements inside
 * parse_statement and are only legal while switch_depth > 0.
 */
static ASTNode *parse_switch_statement(Parser *parser) {
    parser_expect(parser, TOKEN_SWITCH);
    parser_expect(parser, TOKEN_LPAREN);
    ASTNode *expr = parse_expression(parser);
    parser_expect(parser, TOKEN_RPAREN);

    ASTNode *switch_node = ast_new(AST_SWITCH_STATEMENT, NULL);
    ast_add_child(switch_node, expr);

    parser->switch_depth++;
    ASTNode *body = parse_statement(parser);
    parser->switch_depth--;

    ast_add_child(switch_node, body);

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
    /* labeled_statement: <identifier> ":" <statement> */
    if (parser->current.type == TOKEN_IDENTIFIER) {
        int saved_pos = parser->lexer->pos;
        Token saved_token = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        if (parser->current.type == TOKEN_COLON) {
            parser->current = lexer_next_token(parser->lexer);
            ASTNode *node = ast_new(AST_LABEL_STATEMENT, saved_token.value);
            ast_add_child(node, parse_statement(parser));
            return node;
        }
        /* Not a label — restore lexer state */
        parser->lexer->pos = saved_pos;
        parser->current = saved_token;
    }
    /* declaration: <type> <identifier> [ "=" <expression> ] ";"
     * <type>     ::= <integer_type> { "*" }
     *
     * Stage 12-01: zero or more '*' tokens after the integer base type
     * wrap the type in a pointer Type chain. When at least one '*' is
     * consumed, decl_type becomes TYPE_POINTER and full_type carries
     * the head of the chain. */
    if (parser->current.type == TOKEN_CHAR ||
        parser->current.type == TOKEN_SHORT ||
        parser->current.type == TOKEN_INT ||
        parser->current.type == TOKEN_LONG) {
        TypeKind base_kind;
        Type *base_type;
        switch (parser->current.type) {
        case TOKEN_CHAR:  base_kind = TYPE_CHAR;  base_type = type_char();  break;
        case TOKEN_SHORT: base_kind = TYPE_SHORT; base_type = type_short(); break;
        case TOKEN_LONG:  base_kind = TYPE_LONG;  base_type = type_long();  break;
        default:          base_kind = TYPE_INT;   base_type = type_int();   break;
        }
        parser->current = lexer_next_token(parser->lexer);
        Type *full_type = base_type;
        int pointer_levels = 0;
        while (parser->current.type == TOKEN_STAR) {
            full_type = type_pointer(full_type);
            pointer_levels++;
            parser->current = lexer_next_token(parser->lexer);
        }
        Token name = parser_expect(parser, TOKEN_IDENTIFIER);
        ASTNode *decl = ast_new(AST_DECLARATION, name.value);
        /* Stage 13-01: optional "[" <integer_literal> "]" suffix
         * makes this an array declaration. The element type is the
         * (possibly pointer-wrapped) base type.
         *
         * Stage 14-06: the integer-literal size is now optional; an
         * empty `[]` is allowed only when paired with a string-literal
         * initializer, in which case the length is inferred as
         * literal_byte_length + 1. An `=` initializer is also now
         * allowed, but only when the initializer is a string literal
         * and the element type is `char`. All other initializer
         * shapes are rejected with the spec-specified error. */
        if (parser->current.type == TOKEN_LBRACKET) {
            parser->current = lexer_next_token(parser->lexer);
            int has_size = 0;
            int length = 0;
            if (parser->current.type == TOKEN_INT_LITERAL) {
                Token size_tok = parser->current;
                parser->current = lexer_next_token(parser->lexer);
                length = (int)size_tok.long_value;
                if (length <= 0) {
                    fprintf(stderr,
                            "error: array size must be greater than zero\n");
                    exit(1);
                }
                has_size = 1;
            } else if (parser->current.type != TOKEN_RBRACKET) {
                fprintf(stderr,
                        "error: array size must be an integer literal\n");
                exit(1);
            }
            parser_expect(parser, TOKEN_RBRACKET);

            ASTNode *str_init = NULL;
            if (parser->current.type == TOKEN_ASSIGN) {
                parser->current = lexer_next_token(parser->lexer);
                if (parser->current.type != TOKEN_STRING_LITERAL) {
                    if (!has_size) {
                        fprintf(stderr,
                                "error: omitted array size requires string literal initializer\n");
                    } else {
                        fprintf(stderr,
                                "error: array initializers not supported\n");
                    }
                    exit(1);
                }
                if (full_type->kind != TYPE_CHAR) {
                    fprintf(stderr,
                            "error: string initializer only supported for char arrays\n");
                    exit(1);
                }
                Token str_tok = parser->current;
                parser->current = lexer_next_token(parser->lexer);
                str_init = ast_new(AST_STRING_LITERAL, NULL);
                memcpy(str_init->value, str_tok.value, str_tok.length);
                str_init->value[str_tok.length] = '\0';
                str_init->byte_length = str_tok.length;
                str_init->decl_type = TYPE_ARRAY;
                str_init->full_type = type_array(type_char(), str_tok.length + 1);
                int needed = str_init->byte_length + 1;
                if (has_size) {
                    if (length < needed) {
                        fprintf(stderr,
                                "error: array too small for string literal initializer\n");
                        exit(1);
                    }
                } else {
                    length = needed;
                }
            } else if (!has_size) {
                fprintf(stderr,
                        "error: array size required unless initialized from string literal\n");
                exit(1);
            }

            Type *array_type = type_array(full_type, length);
            decl->decl_type = TYPE_ARRAY;
            decl->full_type = array_type;
            if (str_init) {
                ast_add_child(decl, str_init);
            }
            parser_expect(parser, TOKEN_SEMICOLON);
            return decl;
        }
        if (pointer_levels > 0) {
            decl->decl_type = TYPE_POINTER;
            decl->full_type = full_type;
        } else {
            decl->decl_type = base_kind;
        }
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
    if (parser->current.type == TOKEN_CASE) {
        if (parser->switch_depth == 0) {
            fprintf(stderr, "error: 'case' label outside of switch\n");
            exit(1);
        }
        parser->current = lexer_next_token(parser->lexer);
        Token value = parser_expect(parser, TOKEN_INT_LITERAL);
        parser_expect(parser, TOKEN_COLON);
        ASTNode *node = ast_new(AST_CASE_SECTION, NULL);
        ast_add_child(node, ast_new(AST_INT_LITERAL, value.value));
        ast_add_child(node, parse_statement(parser));
        return node;
    }
    if (parser->current.type == TOKEN_DEFAULT) {
        if (parser->switch_depth == 0) {
            fprintf(stderr, "error: 'default' label outside of switch\n");
            exit(1);
        }
        parser->current = lexer_next_token(parser->lexer);
        parser_expect(parser, TOKEN_COLON);
        ASTNode *node = ast_new(AST_DEFAULT_SECTION, NULL);
        ast_add_child(node, parse_statement(parser));
        return node;
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
    if (parser->current.type == TOKEN_GOTO) {
        parser->current = lexer_next_token(parser->lexer);
        Token name = parser_expect(parser, TOKEN_IDENTIFIER);
        parser_expect(parser, TOKEN_SEMICOLON);
        return ast_new(AST_GOTO_STATEMENT, name.value);
    }
    /* expression_stmt (includes assignments, since assignment is now an expression) */
    ASTNode *expr = parse_expression(parser);
    parser_expect(parser, TOKEN_SEMICOLON);
    ASTNode *stmt = ast_new(AST_EXPRESSION_STMT, NULL);
    ast_add_child(stmt, expr);
    return stmt;
}

/*
 * <parameter_declaration> ::= <type> <identifier>
 * <type>                  ::= <integer_type> { "*" }
 *
 * Stage 12-04: a parameter's type may be a pointer. Mirrors the
 * declaration parsing: the integer base type is followed by zero or
 * more '*' tokens. When at least one '*' is consumed the AST_PARAM
 * carries decl_type = TYPE_POINTER and full_type pointing at the
 * pointer chain head.
 */
static ASTNode *parse_parameter_declaration(Parser *parser) {
    TypeKind base_kind;
    Type *base_type;
    switch (parser->current.type) {
    case TOKEN_CHAR:  base_kind = TYPE_CHAR;  base_type = type_char();  break;
    case TOKEN_SHORT: base_kind = TYPE_SHORT; base_type = type_short(); break;
    case TOKEN_LONG:  base_kind = TYPE_LONG;  base_type = type_long();  break;
    case TOKEN_INT:   base_kind = TYPE_INT;   base_type = type_int();   break;
    default:
        fprintf(stderr, "error: expected integer type, got '%s'\n",
                parser->current.value);
        exit(1);
    }
    parser->current = lexer_next_token(parser->lexer);
    Type *full_type = base_type;
    int pointer_levels = 0;
    while (parser->current.type == TOKEN_STAR) {
        full_type = type_pointer(full_type);
        pointer_levels++;
        parser->current = lexer_next_token(parser->lexer);
    }
    Token name = parser_expect(parser, TOKEN_IDENTIFIER);
    ASTNode *param = ast_new(AST_PARAM, name.value);
    if (pointer_levels > 0) {
        param->decl_type = TYPE_POINTER;
        param->full_type = full_type;
    } else {
        param->decl_type = base_kind;
    }
    return param;
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
 * <function> ::= <type> <identifier> "(" [ <parameter_list> ] ")"
 *                ( <block> | ";" )
 * <type>     ::= <integer_type> { "*" }
 *
 * AST layout for a definition: zero or more AST_PARAM children followed by
 * the AST_BLOCK body. A pure declaration has only the AST_PARAM children
 * (no AST_BLOCK).
 *
 * Stage 12-05: zero or more '*' tokens after the integer base type wrap
 * the return type in a pointer Type chain. When at least one '*' is
 * consumed, decl_type becomes TYPE_POINTER and full_type carries the
 * head of the chain.
 */
static ASTNode *parse_function_decl(Parser *parser) {
    TypeKind base_kind;
    Type *base_type;
    switch (parser->current.type) {
    case TOKEN_CHAR:  base_kind = TYPE_CHAR;  base_type = type_char();  break;
    case TOKEN_SHORT: base_kind = TYPE_SHORT; base_type = type_short(); break;
    case TOKEN_LONG:  base_kind = TYPE_LONG;  base_type = type_long();  break;
    case TOKEN_INT:   base_kind = TYPE_INT;   base_type = type_int();   break;
    default:
        fprintf(stderr, "error: expected integer type, got '%s'\n",
                parser->current.value);
        exit(1);
    }
    parser->current = lexer_next_token(parser->lexer);
    Type *full_type = base_type;
    int pointer_levels = 0;
    while (parser->current.type == TOKEN_STAR) {
        full_type = type_pointer(full_type);
        pointer_levels++;
        parser->current = lexer_next_token(parser->lexer);
    }
    TypeKind return_kind = (pointer_levels > 0) ? TYPE_POINTER : base_kind;
    Token name = parser_expect(parser, TOKEN_IDENTIFIER);
    ASTNode *func = ast_new(AST_FUNCTION_DECL, name.value);
    func->decl_type = return_kind;
    if (pointer_levels > 0) {
        func->full_type = full_type;
    }

    parser_expect(parser, TOKEN_LPAREN);
    if (parser->current.type != TOKEN_RPAREN) {
        parse_parameter_list(parser, func);
    }
    parser_expect(parser, TOKEN_RPAREN);

    int param_count = func->child_count;
    int is_definition = (parser->current.type == TOKEN_LBRACE);

    /* Collect the declared parameter types in order so the function
     * signature can be stored on registration. */
    TypeKind param_types[FUNC_MAX_PARAMS];
    if (param_count > FUNC_MAX_PARAMS) {
        fprintf(stderr,
                "error: function '%s' has %d parameters; max supported is %d\n",
                name.value, param_count, FUNC_MAX_PARAMS);
        exit(1);
    }
    for (int i = 0; i < param_count; i++) {
        param_types[i] = func->children[i]->decl_type;
    }

    /* Register before parsing the body so self-calls resolve. The helper
     * also enforces param-count consistency and rejects duplicate
     * definitions. */
    parser_register_function(parser, name.value, param_count, is_definition,
                             return_kind, param_types);

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
