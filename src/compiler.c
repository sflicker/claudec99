/*
 * ccompiler - A minimal C compiler
 * Stage 7.4: Pretty-print AST
 *
 * Pipeline: Source -> Lexer -> Parser (AST) -> Code Generator (x86_64 NASM)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ========================================================================
 * Section 1: Token Definitions
 * ======================================================================== */

typedef enum {
    TOKEN_EOF,
    TOKEN_INT,
    TOKEN_RETURN,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_IDENTIFIER,
    TOKEN_INT_LITERAL,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_SEMICOLON,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_BANG,
    TOKEN_ASSIGN,
    TOKEN_PLUS_ASSIGN,
    TOKEN_MINUS_ASSIGN,
    TOKEN_INCREMENT,
    TOKEN_DECREMENT,
    TOKEN_EQ,
    TOKEN_NE,
    TOKEN_LT,
    TOKEN_LE,
    TOKEN_GT,
    TOKEN_GE,
    TOKEN_UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    char value[256];
    int int_value;
} Token;

/* ========================================================================
 * Section 2: AST Node Definitions
 * ======================================================================== */

typedef enum {
    AST_PROGRAM,
    AST_FUNCTION_DECL,
    AST_RETURN_STATEMENT,
    AST_INT_LITERAL,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_IF_STATEMENT,
    AST_WHILE_STATEMENT,
    AST_FOR_STATEMENT,
    AST_BLOCK,
    AST_EXPRESSION_STMT,
    AST_DECLARATION,
    AST_ASSIGNMENT,
    AST_VAR_REF,
    AST_PREFIX_INC_DEC,
    AST_POSTFIX_INC_DEC
} ASTNodeType;

#define AST_MAX_CHILDREN 64

typedef struct ASTNode {
    ASTNodeType type;
    char value[256];
    struct ASTNode *children[AST_MAX_CHILDREN];
    int child_count;
} ASTNode;

static ASTNode *ast_new(ASTNodeType type, const char *value) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "error: out of memory\n");
        exit(1);
    }
    node->type = type;
    if (value) {
        strncpy(node->value, value, sizeof(node->value) - 1);
    }
    return node;
}

static void ast_add_child(ASTNode *parent, ASTNode *child) {
    if (parent->child_count < AST_MAX_CHILDREN) {
        parent->children[parent->child_count++] = child;
    }
}

static void ast_free(ASTNode *node) {
    if (!node) return;
    for (int i = 0; i < node->child_count; i++) {
        ast_free(node->children[i]);
    }
    free(node);
}

/* ========================================================================
 * Section 3: Lexer
 * ======================================================================== */

typedef struct {
    const char *source;
    int pos;
} Lexer;

static void lexer_init(Lexer *lexer, const char *source) {
    lexer->source = source;
    lexer->pos = 0;
}

static void lexer_skip_whitespace(Lexer *lexer) {
    while (lexer->source[lexer->pos] && isspace(lexer->source[lexer->pos])) {
        lexer->pos++;
    }
}

static Token lexer_next_token(Lexer *lexer) {
    Token token = {TOKEN_EOF, ""};

    lexer_skip_whitespace(lexer);

    char c = lexer->source[lexer->pos];
    if (c == '\0') {
        return token;
    }

    /* Single-character tokens */
    if (c == '(') { token.type = TOKEN_LPAREN;    token.value[0] = c; lexer->pos++; return token; }
    if (c == ')') { token.type = TOKEN_RPAREN;    token.value[0] = c; lexer->pos++; return token; }
    if (c == '{') { token.type = TOKEN_LBRACE;    token.value[0] = c; lexer->pos++; return token; }
    if (c == '}') { token.type = TOKEN_RBRACE;    token.value[0] = c; lexer->pos++; return token; }
    if (c == ';') { token.type = TOKEN_SEMICOLON; token.value[0] = c; lexer->pos++; return token; }
    if (c == '+') {
        if (lexer->source[lexer->pos + 1] == '+') { token.type = TOKEN_INCREMENT; strcpy(token.value, "++"); lexer->pos += 2; return token; }
        if (lexer->source[lexer->pos + 1] == '=') { token.type = TOKEN_PLUS_ASSIGN; strcpy(token.value, "+="); lexer->pos += 2; return token; }
        token.type = TOKEN_PLUS; token.value[0] = c; lexer->pos++; return token;
    }
    if (c == '-') {
        if (lexer->source[lexer->pos + 1] == '-') { token.type = TOKEN_DECREMENT; strcpy(token.value, "--"); lexer->pos += 2; return token; }
        if (lexer->source[lexer->pos + 1] == '=') { token.type = TOKEN_MINUS_ASSIGN; strcpy(token.value, "-="); lexer->pos += 2; return token; }
        token.type = TOKEN_MINUS; token.value[0] = c; lexer->pos++; return token;
    }
    if (c == '*') { token.type = TOKEN_STAR;       token.value[0] = c; lexer->pos++; return token; }
    if (c == '/') { token.type = TOKEN_SLASH;      token.value[0] = c; lexer->pos++; return token; }

    /* Two-character or single-character relational/equality tokens */
    char n = lexer->source[lexer->pos + 1];
    if (c == '=' && n == '=') { token.type = TOKEN_EQ; strcpy(token.value, "=="); lexer->pos += 2; return token; }
    if (c == '=') { token.type = TOKEN_ASSIGN; token.value[0] = c; lexer->pos++; return token; }
    if (c == '!' && n == '=') { token.type = TOKEN_NE; strcpy(token.value, "!="); lexer->pos += 2; return token; }
    if (c == '!') { token.type = TOKEN_BANG; token.value[0] = c; lexer->pos++; return token; }
    if (c == '<' && n == '=') { token.type = TOKEN_LE; strcpy(token.value, "<="); lexer->pos += 2; return token; }
    if (c == '>' && n == '=') { token.type = TOKEN_GE; strcpy(token.value, ">="); lexer->pos += 2; return token; }
    if (c == '<') { token.type = TOKEN_LT; token.value[0] = c; lexer->pos++; return token; }
    if (c == '>') { token.type = TOKEN_GT; token.value[0] = c; lexer->pos++; return token; }

    /* Integer literals */
    if (isdigit(c)) {
        int i = 0;
        while (isdigit(lexer->source[lexer->pos]) && i < 255) {
            token.value[i++] = lexer->source[lexer->pos++];
        }
        token.value[i] = '\0';
        token.type = TOKEN_INT_LITERAL;
        token.int_value = atoi(token.value);
        return token;
    }

    /* Keywords and identifiers */
    if (isalpha(c) || c == '_') {
        int i = 0;
        while ((isalnum(lexer->source[lexer->pos]) || lexer->source[lexer->pos] == '_') && i < 255) {
            token.value[i++] = lexer->source[lexer->pos++];
        }
        token.value[i] = '\0';

        if (strcmp(token.value, "int") == 0) {
            token.type = TOKEN_INT;
        } else if (strcmp(token.value, "return") == 0) {
            token.type = TOKEN_RETURN;
        } else if (strcmp(token.value, "if") == 0) {
            token.type = TOKEN_IF;
        } else if (strcmp(token.value, "else") == 0) {
            token.type = TOKEN_ELSE;
        } else if (strcmp(token.value, "while") == 0) {
            token.type = TOKEN_WHILE;
        } else if (strcmp(token.value, "for") == 0) {
            token.type = TOKEN_FOR;
        } else {
            token.type = TOKEN_IDENTIFIER;
        }
        return token;
    }

    /* Unknown token */
    token.type = TOKEN_UNKNOWN;
    token.value[0] = c;
    lexer->pos++;
    return token;
}

/* ========================================================================
 * Section 4: Parser (Recursive Descent)
 * ======================================================================== */

typedef struct {
    Lexer *lexer;
    Token current;
} Parser;

static void parser_init(Parser *parser, Lexer *lexer) {
    parser->lexer = lexer;
    parser->current = lexer_next_token(lexer);
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
 * <expression> ::= <assignment_expression>
 *
 * <assignment_expression> ::= <identifier> "=" <assignment_expression>
 *                            | <identifier> "+=" <assignment_expression>
 *                            | <identifier> "-=" <assignment_expression>
 *                            | <equality_expression>
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
    return parse_equality(parser);
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
    ASTNode *body = parse_statement(parser);

    ASTNode *while_node = ast_new(AST_WHILE_STATEMENT, NULL);
    ast_add_child(while_node, condition);
    ast_add_child(while_node, body);

    return while_node;
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

    ASTNode *body = parse_statement(parser);

    /* Store as children — NULLs are stored directly */
    for_node->children[0] = init;
    for_node->children[1] = condition;
    for_node->children[2] = update;
    for_node->children[3] = body;
    for_node->child_count = 4;

    return for_node;
}

/*
 * <statement> ::= <declaration>
 *               | <assignment_statement>
 *               | <return_statement>
 *               | <if_statement>
 *               | <while_statement>
 *               | <for_statement>
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
    if (parser->current.type == TOKEN_FOR) {
        return parse_for_statement(parser);
    }
    if (parser->current.type == TOKEN_LBRACE) {
        return parse_block(parser);
    }
    /* expression_stmt (includes assignments, since assignment is now an expression) */
    ASTNode *expr = parse_expression(parser);
    parser_expect(parser, TOKEN_SEMICOLON);
    ASTNode *stmt = ast_new(AST_EXPRESSION_STMT, NULL);
    ast_add_child(stmt, expr);
    return stmt;
}

/*
 * <function> ::= "int" <identifier> "(" ")" <block>
 */
static ASTNode *parse_function_decl(Parser *parser) {
    parser_expect(parser, TOKEN_INT);
    Token name = parser_expect(parser, TOKEN_IDENTIFIER);
    parser_expect(parser, TOKEN_LPAREN);
    parser_expect(parser, TOKEN_RPAREN);
    ASTNode *body = parse_block(parser);

    ASTNode *func = ast_new(AST_FUNCTION_DECL, name.value);
    ast_add_child(func, body);
    return func;
}

/*
 * <program> ::= <function_decl>
 */
static ASTNode *parse_program(Parser *parser) {
    ASTNode *func = parse_function_decl(parser);
    parser_expect(parser, TOKEN_EOF);

    ASTNode *program = ast_new(AST_PROGRAM, NULL);
    ast_add_child(program, func);
    return program;
}

/* ========================================================================
 * Section 5: AST Pretty Printer
 * ======================================================================== */

static const char *operator_name(const char *op) {
    if (strcmp(op, "+") == 0)  return "ADD";
    if (strcmp(op, "-") == 0)  return "SUBTRACT";
    if (strcmp(op, "*") == 0)  return "MULTIPLY";
    if (strcmp(op, "/") == 0)  return "DIVIDE";
    if (strcmp(op, "==") == 0) return "EQUAL";
    if (strcmp(op, "!=") == 0) return "NOTEQUAL";
    if (strcmp(op, "<") == 0)  return "LESSTHAN";
    if (strcmp(op, "<=") == 0) return "LESSTHANOREQUAL";
    if (strcmp(op, ">") == 0)  return "GREATERTHAN";
    if (strcmp(op, ">=") == 0) return "GREATERTHANOREQUAL";
    if (strcmp(op, "!") == 0)  return "NOT";
    if (strcmp(op, "++") == 0) return "INCREMENT";
    if (strcmp(op, "--") == 0) return "DECREMENT";
    return op;
}

static void ast_print_indent(int depth) {
    for (int i = 0; i < depth * 2; i++) {
        putchar(' ');
    }
}

static void ast_pretty_print(ASTNode *node, int depth) {
    if (!node) return;

    ast_print_indent(depth);

    switch (node->type) {
    case AST_PROGRAM:
        printf("ProgramDecl:\n");
        break;
    case AST_FUNCTION_DECL:
        printf("FunctionDecl: %s\n", node->value);
        break;
    case AST_BLOCK:
        printf("Block\n");
        break;
    case AST_DECLARATION:
        printf("VariableDeclaration: %s\n", node->value);
        break;
    case AST_RETURN_STATEMENT:
        printf("ReturnStmt:\n");
        break;
    case AST_INT_LITERAL:
        printf("IntLiteral: %s\n", node->value);
        break;
    case AST_VAR_REF:
        printf("VariableExpression: %s\n", node->value);
        break;
    case AST_BINARY_OP:
        printf("Binary: %s\n", operator_name(node->value));
        break;
    case AST_UNARY_OP:
        printf("Unary: %s\n", operator_name(node->value));
        break;
    case AST_ASSIGNMENT:
        printf("Assignment: %s\n", node->value);
        break;
    case AST_EXPRESSION_STMT:
        printf("ExpressionStatement\n");
        break;
    case AST_IF_STATEMENT:
        printf("IfStmt:\n");
        break;
    case AST_WHILE_STATEMENT:
        printf("WhileStmt:\n");
        break;
    case AST_FOR_STATEMENT:
        printf("ForStmt:\n");
        /* For statements have NULL children for omitted clauses */
        for (int i = 0; i < node->child_count; i++) {
            if (node->children[i]) {
                ast_pretty_print(node->children[i], depth + 1);
            }
        }
        return;
    case AST_PREFIX_INC_DEC:
        printf("PrefixIncDec: %s\n", operator_name(node->value));
        break;
    case AST_POSTFIX_INC_DEC:
        printf("PostfixIncDec: %s\n", operator_name(node->value));
        break;
    }

    for (int i = 0; i < node->child_count; i++) {
        ast_pretty_print(node->children[i], depth + 1);
    }
}

/* ========================================================================
 * Section 6: Code Generator (x86_64 NASM Intel syntax)
 * ======================================================================== */

#define MAX_LOCALS 64

typedef struct {
    char name[256];
    int offset;
} LocalVar;

typedef struct {
    FILE *output;
    int label_count;
    LocalVar locals[MAX_LOCALS];
    int local_count;
    int stack_offset;
} CodeGen;

static void codegen_init(CodeGen *cg, FILE *output) {
    cg->output = output;
    cg->label_count = 0;
    cg->local_count = 0;
    cg->stack_offset = 0;
}

static int codegen_find_var(CodeGen *cg, const char *name) {
    for (int i = 0; i < cg->local_count; i++) {
        if (strcmp(cg->locals[i].name, name) == 0)
            return cg->locals[i].offset;
    }
    return 0; /* not found (valid offsets start at 4) */
}

static int codegen_add_var(CodeGen *cg, const char *name) {
    cg->stack_offset += 4;
    strncpy(cg->locals[cg->local_count].name, name, 255);
    cg->locals[cg->local_count].name[255] = '\0';
    cg->locals[cg->local_count].offset = cg->stack_offset;
    cg->local_count++;
    return cg->stack_offset;
}

static int count_declarations(ASTNode *block) {
    int count = 0;
    for (int i = 0; i < block->child_count; i++) {
        if (block->children[i]->type == AST_DECLARATION)
            count++;
    }
    return count;
}

static void codegen_expression(CodeGen *cg, ASTNode *node) {
    if (node->type == AST_INT_LITERAL) {
        fprintf(cg->output, "    mov eax, %s\n", node->value);
        return;
    }
    if (node->type == AST_VAR_REF) {
        int offset = codegen_find_var(cg, node->value);
        if (offset == 0) {
            fprintf(stderr, "error: undeclared variable '%s'\n", node->value);
            exit(1);
        }
        fprintf(cg->output, "    mov eax, [rbp - %d]\n", offset);
        return;
    }
    if (node->type == AST_ASSIGNMENT) {
        int offset = codegen_find_var(cg, node->value);
        if (offset == 0) {
            fprintf(stderr, "error: undeclared variable '%s'\n", node->value);
            exit(1);
        }
        codegen_expression(cg, node->children[0]);
        fprintf(cg->output, "    mov [rbp - %d], eax\n", offset);
        return;
    }
    if (node->type == AST_UNARY_OP) {
        codegen_expression(cg, node->children[0]);
        const char *op = node->value;
        if (strcmp(op, "-") == 0) {
            fprintf(cg->output, "    neg eax\n");
        } else if (strcmp(op, "!") == 0) {
            fprintf(cg->output, "    cmp eax, 0\n");
            fprintf(cg->output, "    sete al\n");
            fprintf(cg->output, "    movzx eax, al\n");
        }
        /* unary + is a no-op */
        return;
    }
    if (node->type == AST_PREFIX_INC_DEC) {
        /* ++x or --x: update variable, result is new value */
        const char *var_name = node->children[0]->value;
        int offset = codegen_find_var(cg, var_name);
        if (offset == 0) {
            fprintf(stderr, "error: undeclared variable '%s'\n", var_name);
            exit(1);
        }
        fprintf(cg->output, "    mov eax, [rbp - %d]\n", offset);
        if (strcmp(node->value, "++") == 0) {
            fprintf(cg->output, "    add eax, 1\n");
        } else {
            fprintf(cg->output, "    sub eax, 1\n");
        }
        fprintf(cg->output, "    mov [rbp - %d], eax\n", offset);
        return;
    }
    if (node->type == AST_POSTFIX_INC_DEC) {
        /* x++ or x--: result is old value, then update variable */
        const char *var_name = node->children[0]->value;
        int offset = codegen_find_var(cg, var_name);
        if (offset == 0) {
            fprintf(stderr, "error: undeclared variable '%s'\n", var_name);
            exit(1);
        }
        fprintf(cg->output, "    mov eax, [rbp - %d]\n", offset);
        fprintf(cg->output, "    mov ecx, eax\n");  /* save old value */
        if (strcmp(node->value, "++") == 0) {
            fprintf(cg->output, "    add ecx, 1\n");
        } else {
            fprintf(cg->output, "    sub ecx, 1\n");
        }
        fprintf(cg->output, "    mov [rbp - %d], ecx\n", offset);
        /* eax still holds the old value */
        return;
    }
    if (node->type == AST_BINARY_OP) {
        /* Evaluate left into eax, push it */
        codegen_expression(cg, node->children[0]);
        fprintf(cg->output, "    push rax\n");
        /* Evaluate right into eax */
        codegen_expression(cg, node->children[1]);
        /* Pop left into ecx; now ecx=left, eax=right */
        fprintf(cg->output, "    pop rcx\n");
        const char *op = node->value;
        if (strcmp(op, "+") == 0) {
            fprintf(cg->output, "    add eax, ecx\n");
        } else if (strcmp(op, "-") == 0) {
            /* left - right: ecx - eax */
            fprintf(cg->output, "    sub ecx, eax\n");
            fprintf(cg->output, "    mov eax, ecx\n");
        } else if (strcmp(op, "*") == 0) {
            fprintf(cg->output, "    imul eax, ecx\n");
        } else if (strcmp(op, "/") == 0) {
            /* left / right: ecx / eax */
            fprintf(cg->output, "    xchg eax, ecx\n");
            fprintf(cg->output, "    cdq\n");
            fprintf(cg->output, "    idiv ecx\n");
        } else {
            /* Comparisons: compare ecx (left) with eax (right), set al, zero-extend */
            const char *setcc = NULL;
            if      (strcmp(op, "==") == 0) setcc = "sete";
            else if (strcmp(op, "!=") == 0) setcc = "setne";
            else if (strcmp(op, "<")  == 0) setcc = "setl";
            else if (strcmp(op, "<=") == 0) setcc = "setle";
            else if (strcmp(op, ">")  == 0) setcc = "setg";
            else if (strcmp(op, ">=") == 0) setcc = "setge";
            fprintf(cg->output, "    cmp ecx, eax\n");
            fprintf(cg->output, "    %s al\n", setcc);
            fprintf(cg->output, "    movzx eax, al\n");
        }
        return;
    }
}

static void codegen_statement(CodeGen *cg, ASTNode *node, int is_main, int allows_decl) {
    if (node->type == AST_DECLARATION) {
        if (!allows_decl) {
            fprintf(stderr, "error: variable declaration not allowed in nested scope\n");
            exit(1);
        }
        if (codegen_find_var(cg, node->value) != 0) {
            fprintf(stderr, "error: duplicate declaration of variable '%s'\n", node->value);
            exit(1);
        }
        int offset = codegen_add_var(cg, node->value);
        if (node->child_count > 0) {
            codegen_expression(cg, node->children[0]);
            fprintf(cg->output, "    mov [rbp - %d], eax\n", offset);
        }
    } else if (node->type == AST_RETURN_STATEMENT) {
        codegen_expression(cg, node->children[0]);
        if (is_main) {
            fprintf(cg->output, "    mov edi, eax\n");
            fprintf(cg->output, "    mov eax, 60\n");
            fprintf(cg->output, "    syscall\n");
        } else {
            fprintf(cg->output, "    ret\n");
        }
    } else if (node->type == AST_IF_STATEMENT) {
        int label_id = cg->label_count++;
        codegen_expression(cg, node->children[0]);
        fprintf(cg->output, "    cmp eax, 0\n");
        if (node->child_count == 3) {
            /* if/else */
            fprintf(cg->output, "    je .L_else_%d\n", label_id);
            codegen_statement(cg, node->children[1], is_main, 0);
            fprintf(cg->output, "    jmp .L_end_%d\n", label_id);
            fprintf(cg->output, ".L_else_%d:\n", label_id);
            codegen_statement(cg, node->children[2], is_main, 0);
            fprintf(cg->output, ".L_end_%d:\n", label_id);
        } else {
            /* if without else */
            fprintf(cg->output, "    je .L_end_%d\n", label_id);
            codegen_statement(cg, node->children[1], is_main, 0);
            fprintf(cg->output, ".L_end_%d:\n", label_id);
        }
    } else if (node->type == AST_WHILE_STATEMENT) {
        int label_id = cg->label_count++;
        fprintf(cg->output, ".L_while_start_%d:\n", label_id);
        codegen_expression(cg, node->children[0]);
        fprintf(cg->output, "    cmp eax, 0\n");
        fprintf(cg->output, "    je .L_while_end_%d\n", label_id);
        codegen_statement(cg, node->children[1], is_main, 0);
        fprintf(cg->output, "    jmp .L_while_start_%d\n", label_id);
        fprintf(cg->output, ".L_while_end_%d:\n", label_id);
    } else if (node->type == AST_FOR_STATEMENT) {
        /* children: [0]=init, [1]=condition, [2]=update, [3]=body (any may be NULL except body) */
        int label_id = cg->label_count++;
        if (node->children[0]) {
            codegen_expression(cg, node->children[0]);
        }
        fprintf(cg->output, ".L_for_start_%d:\n", label_id);
        if (node->children[1]) {
            codegen_expression(cg, node->children[1]);
            fprintf(cg->output, "    cmp eax, 0\n");
            fprintf(cg->output, "    je .L_for_end_%d\n", label_id);
        }
        codegen_statement(cg, node->children[3], is_main, 0);
        if (node->children[2]) {
            codegen_expression(cg, node->children[2]);
        }
        fprintf(cg->output, "    jmp .L_for_start_%d\n", label_id);
        fprintf(cg->output, ".L_for_end_%d:\n", label_id);
    } else if (node->type == AST_BLOCK) {
        for (int i = 0; i < node->child_count; i++) {
            codegen_statement(cg, node->children[i], is_main, 0);
        }
    } else if (node->type == AST_EXPRESSION_STMT) {
        codegen_expression(cg, node->children[0]);
    }
}

static void codegen_function(CodeGen *cg, ASTNode *node) {
    if (node->type == AST_FUNCTION_DECL) {
        int is_main = (strcmp(node->value, "main") == 0);
        ASTNode *body = node->children[0];

        /* Reset per-function symbol table */
        cg->local_count = 0;
        cg->stack_offset = 0;

        /* Compute stack space for local variables */
        int num_vars = count_declarations(body);
        int stack_size = num_vars * 4;
        if (stack_size % 16 != 0)
            stack_size = (stack_size + 15) & ~15;

        /* Function label and prologue */
        fprintf(cg->output, "global %s\n", node->value);
        fprintf(cg->output, "%s:\n", node->value);
        if (num_vars > 0) {
            fprintf(cg->output, "    push rbp\n");
            fprintf(cg->output, "    mov rbp, rsp\n");
            fprintf(cg->output, "    sub rsp, %d\n", stack_size);
        }

        /* Generate body statements (allows_decl=1 for outermost scope) */
        for (int i = 0; i < body->child_count; i++) {
            codegen_statement(cg, body->children[i], is_main, 1);
        }
    }
}

static void codegen_program(CodeGen *cg, ASTNode *node) {
    fprintf(cg->output, "section .text\n");
    if (node->type == AST_PROGRAM) {
        for (int i = 0; i < node->child_count; i++) {
            codegen_function(cg, node->children[i]);
        }
    }
}

/* ========================================================================
 * Section 7: File I/O Utilities
 * ======================================================================== */

static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "error: could not open '%s'\n", path);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = malloc(len + 1);
    if (!buf) {
        fprintf(stderr, "error: out of memory\n");
        fclose(f);
        exit(1);
    }
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);
    return buf;
}

static void make_output_path(char *out, size_t out_size, const char *input_path) {
    /* Find the basename: skip any directory separators */
    const char *base = strrchr(input_path, '/');
    base = base ? base + 1 : input_path;

    /* Copy basename and replace extension with .asm */
    strncpy(out, base, out_size - 1);
    out[out_size - 1] = '\0';

    char *dot = strrchr(out, '.');
    if (dot) {
        *dot = '\0';
    }
    strncat(out, ".asm", out_size - strlen(out) - 1);
}

/* ========================================================================
 * Section 8: Main
 * ======================================================================== */

int main(int argc, char *argv[]) {
    int print_ast = 0;
    const char *source_file = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--print-ast") == 0) {
            print_ast = 1;
        } else if (!source_file) {
            source_file = argv[i];
        } else {
            fprintf(stderr, "usage: ccompiler [--print-ast] <source.c>\n");
            return 1;
        }
    }

    if (!source_file) {
        fprintf(stderr, "usage: ccompiler [--print-ast] <source.c>\n");
        return 1;
    }

    /* Read source */
    char *source = read_file(source_file);

    /* Lex + Parse */
    Lexer lexer;
    lexer_init(&lexer, source);

    Parser parser;
    parser_init(&parser, &lexer);

    ASTNode *ast = parse_program(&parser);

    if (print_ast) {
        ast_pretty_print(ast, 0);
        ast_free(ast);
        free(source);
        return 0;
    }

    /* Generate output */
    char output_path[512];
    make_output_path(output_path, sizeof(output_path), source_file);

    FILE *out = fopen(output_path, "w");
    if (!out) {
        fprintf(stderr, "error: could not create '%s'\n", output_path);
        free(source);
        ast_free(ast);
        return 1;
    }

    CodeGen cg;
    codegen_init(&cg, out);
    codegen_program(&cg, ast);

    fclose(out);
    ast_free(ast);
    free(source);

    printf("compiled: %s -> %s\n", source_file, output_path);
    return 0;
}
