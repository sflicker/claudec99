/*
 * ccompiler - A minimal C compiler
 * Stage 1: Single function with integer return
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
    TOKEN_IDENTIFIER,
    TOKEN_INT_LITERAL,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_SEMICOLON,
    TOKEN_UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    char value[256];
} Token;

/* ========================================================================
 * Section 2: AST Node Definitions
 * ======================================================================== */

typedef enum {
    AST_PROGRAM,
    AST_FUNCTION_DECL,
    AST_RETURN_STATEMENT,
    AST_INT_LITERAL
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    char value[256];
    struct ASTNode *children[4];
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
    if (parent->child_count < 4) {
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

    /* Integer literals */
    if (isdigit(c)) {
        int i = 0;
        while (isdigit(lexer->source[lexer->pos]) && i < 255) {
            token.value[i++] = lexer->source[lexer->pos++];
        }
        token.value[i] = '\0';
        token.type = TOKEN_INT_LITERAL;
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
 * <expression> ::= <integer_literal>
 */
static ASTNode *parse_expression(Parser *parser) {
    Token token = parser_expect(parser, TOKEN_INT_LITERAL);
    return ast_new(AST_INT_LITERAL, token.value);
}

/*
 * <statement> ::= "return" <expression> ";"
 */
static ASTNode *parse_statement(Parser *parser) {
    parser_expect(parser, TOKEN_RETURN);
    ASTNode *expr = parse_expression(parser);
    parser_expect(parser, TOKEN_SEMICOLON);

    ASTNode *stmt = ast_new(AST_RETURN_STATEMENT, NULL);
    ast_add_child(stmt, expr);
    return stmt;
}

/*
 * <function_decl> ::= "int" <identifier> "(" ")" "{" <statement> "}"
 */
static ASTNode *parse_function_decl(Parser *parser) {
    parser_expect(parser, TOKEN_INT);
    Token name = parser_expect(parser, TOKEN_IDENTIFIER);
    parser_expect(parser, TOKEN_LPAREN);
    parser_expect(parser, TOKEN_RPAREN);
    parser_expect(parser, TOKEN_LBRACE);
    ASTNode *body = parse_statement(parser);
    parser_expect(parser, TOKEN_RBRACE);

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
 * Section 5: Code Generator (x86_64 NASM Intel syntax)
 * ======================================================================== */

typedef struct {
    FILE *output;
} CodeGen;

static void codegen_init(CodeGen *cg, FILE *output) {
    cg->output = output;
}

static void codegen_expression(CodeGen *cg, ASTNode *node) {
    if (node->type == AST_INT_LITERAL) {
        fprintf(cg->output, "    mov eax, %s\n", node->value);
    }
}

static void codegen_statement(CodeGen *cg, ASTNode *node, int is_main) {
    if (node->type == AST_RETURN_STATEMENT) {
        codegen_expression(cg, node->children[0]);
        if (is_main) {
            fprintf(cg->output, "    mov edi, eax\n");
            fprintf(cg->output, "    mov eax, 60\n");
            fprintf(cg->output, "    syscall\n");
        } else {
            fprintf(cg->output, "    ret\n");
        }
    }
}

static void codegen_function(CodeGen *cg, ASTNode *node) {
    if (node->type == AST_FUNCTION_DECL) {
        fprintf(cg->output, "global %s\n", node->value);
        fprintf(cg->output, "%s:\n", node->value);
        int is_main = (strcmp(node->value, "main") == 0);
        codegen_statement(cg, node->children[0], is_main);
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
 * Section 6: File I/O Utilities
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
 * Section 7: Main
 * ======================================================================== */

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: ccompiler <source.c>\n");
        return 1;
    }

    /* Read source */
    char *source = read_file(argv[1]);

    /* Lex + Parse */
    Lexer lexer;
    lexer_init(&lexer, source);

    Parser parser;
    parser_init(&parser, &lexer);

    ASTNode *ast = parse_program(&parser);

    /* Generate output */
    char output_path[512];
    make_output_path(output_path, sizeof(output_path), argv[1]);

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

    printf("compiled: %s -> %s\n", argv[1], output_path);
    return 0;
}
