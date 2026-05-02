/*
 * ccompiler - A minimal C compiler
 *
 * Pipeline: Source -> Lexer -> Parser (AST) -> Code Generator (x86_64 NASM)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "ast_pretty_printer.h"
#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include "token.h"
#include "util.h"

static const char *token_type_name(TokenType t) {
    switch (t) {
        case TOKEN_EOF:           return "TOKEN_EOF";
        case TOKEN_INT:           return "TOKEN_INT";
        case TOKEN_CHAR:          return "TOKEN_CHAR";
        case TOKEN_SHORT:         return "TOKEN_SHORT";
        case TOKEN_LONG:          return "TOKEN_LONG";
        case TOKEN_RETURN:        return "TOKEN_RETURN";
        case TOKEN_IF:            return "TOKEN_IF";
        case TOKEN_ELSE:          return "TOKEN_ELSE";
        case TOKEN_WHILE:         return "TOKEN_WHILE";
        case TOKEN_DO:            return "TOKEN_DO";
        case TOKEN_FOR:           return "TOKEN_FOR";
        case TOKEN_BREAK:         return "TOKEN_BREAK";
        case TOKEN_CONTINUE:      return "TOKEN_CONTINUE";
        case TOKEN_SWITCH:        return "TOKEN_SWITCH";
        case TOKEN_DEFAULT:       return "TOKEN_DEFAULT";
        case TOKEN_CASE:          return "TOKEN_CASE";
        case TOKEN_GOTO:          return "TOKEN_GOTO";
        case TOKEN_IDENTIFIER:    return "TOKEN_IDENTIFIER";
        case TOKEN_INT_LITERAL:   return "TOKEN_INT_LITERAL";
        case TOKEN_STRING_LITERAL: return "TOKEN_STRING_LITERAL";
        case TOKEN_CHAR_LITERAL:  return "TOKEN_CHAR_LITERAL";
        case TOKEN_LPAREN:        return "TOKEN_LPAREN";
        case TOKEN_RPAREN:        return "TOKEN_RPAREN";
        case TOKEN_LBRACE:        return "TOKEN_LBRACE";
        case TOKEN_RBRACE:        return "TOKEN_RBRACE";
        case TOKEN_LBRACKET:      return "TOKEN_LBRACKET";
        case TOKEN_RBRACKET:      return "TOKEN_RBRACKET";
        case TOKEN_SEMICOLON:     return "TOKEN_SEMICOLON";
        case TOKEN_COLON:         return "TOKEN_COLON";
        case TOKEN_COMMA:         return "TOKEN_COMMA";
        case TOKEN_PLUS:          return "TOKEN_PLUS";
        case TOKEN_MINUS:         return "TOKEN_MINUS";
        case TOKEN_STAR:          return "TOKEN_STAR";
        case TOKEN_SLASH:         return "TOKEN_SLASH";
        case TOKEN_PERCENT:       return "TOKEN_PERCENT";
        case TOKEN_BANG:          return "TOKEN_BANG";
        case TOKEN_TILDE:         return "TOKEN_TILDE";
        case TOKEN_ASSIGN:        return "TOKEN_ASSIGN";
        case TOKEN_PLUS_ASSIGN:   return "TOKEN_PLUS_ASSIGN";
        case TOKEN_MINUS_ASSIGN:  return "TOKEN_MINUS_ASSIGN";
        case TOKEN_INCREMENT:     return "TOKEN_INCREMENT";
        case TOKEN_DECREMENT:     return "TOKEN_DECREMENT";
        case TOKEN_EQ:            return "TOKEN_EQ";
        case TOKEN_NE:            return "TOKEN_NE";
        case TOKEN_LT:            return "TOKEN_LT";
        case TOKEN_LE:            return "TOKEN_LE";
        case TOKEN_GT:            return "TOKEN_GT";
        case TOKEN_GE:            return "TOKEN_GE";
        case TOKEN_AND_AND:       return "TOKEN_AND_AND";
        case TOKEN_OR_OR:         return "TOKEN_OR_OR";
        case TOKEN_AMPERSAND:     return "TOKEN_AMPERSAND";
        case TOKEN_UNKNOWN:       return "TOKEN_UNKNOWN";
    }
    return "TOKEN_UNKNOWN";
}

/* Print a single token row.
 *
 * Format:
 *   [N] TOKEN:: <value>  TOKEN_TYPE: <type>
 *
 * - Index field is left-justified to `index_width` chars (file-wide
 *   max digit count) so all rows align.
 * - Value field is left-justified to 18 chars: up to 15 chars of the
 *   token text; if longer, the first 15 chars followed by "...".
 *
 * Stage 14-05: a string literal's decoded payload may contain bytes
 * that would break the line-oriented output (embedded NUL, newline,
 * tab, CR) or the surrounding quotes/backslash. Re-escape those
 * bytes back to source form before applying the width logic so the
 * printed value remains readable and the buffer length still maps
 * onto a single line.
 */
static void print_token_row(int index, int index_width, const Token *tok) {
    char display[512];
    size_t display_len = 0;
    if (tok->type == TOKEN_STRING_LITERAL ||
        tok->type == TOKEN_CHAR_LITERAL) {
        for (int i = 0; i < tok->length && display_len + 2 < sizeof(display); i++) {
            unsigned char b = (unsigned char)tok->value[i];
            switch (b) {
            case '\n':
                display[display_len++] = '\\';
                display[display_len++] = 'n';
                break;
            case '\t':
                display[display_len++] = '\\';
                display[display_len++] = 't';
                break;
            case '\r':
                display[display_len++] = '\\';
                display[display_len++] = 'r';
                break;
            case '\\':
                display[display_len++] = '\\';
                display[display_len++] = '\\';
                break;
            case '"':
                display[display_len++] = '\\';
                display[display_len++] = '"';
                break;
            case '\'':
                display[display_len++] = '\\';
                display[display_len++] = '\'';
                break;
            case 0:
                display[display_len++] = '\\';
                display[display_len++] = '0';
                break;
            default:
                display[display_len++] = (char)b;
                break;
            }
        }
        display[display_len] = '\0';
    } else {
        size_t src_len = strlen(tok->value);
        if (src_len >= sizeof(display)) src_len = sizeof(display) - 1;
        memcpy(display, tok->value, src_len);
        display[src_len] = '\0';
        display_len = src_len;
    }

    char value_buf[19];
    if (display_len > 15) {
        memcpy(value_buf, display, 15);
        memcpy(value_buf + 15, "...", 3);
        value_buf[18] = '\0';
    } else {
        memcpy(value_buf, display, display_len);
        for (size_t i = display_len; i < 18; i++) {
            value_buf[i] = ' ';
        }
        value_buf[18] = '\0';
    }
    printf("[%-*d] TOKEN:: %s  TOKEN_TYPE: %s\n",
           index_width, index, value_buf, token_type_name(tok->type));
}

static int digit_width(int n) {
    int w = 1;
    while (n >= 10) {
        n /= 10;
        w++;
    }
    return w;
}

static void print_tokens_mode(const char *source) {
    Lexer lexer;
    lexer_init(&lexer, source);

    /* Lex into a growable buffer so we know the final token count
     * (including TOKEN_EOF) before printing — needed for the
     * index-column width. */
    size_t cap = 64;
    size_t n = 0;
    Token *tokens = malloc(cap * sizeof(Token));
    if (!tokens) {
        fprintf(stderr, "error: out of memory in --print-tokens\n");
        exit(1);
    }
    for (;;) {
        if (n == cap) {
            cap *= 2;
            Token *grown = realloc(tokens, cap * sizeof(Token));
            if (!grown) {
                fprintf(stderr, "error: out of memory in --print-tokens\n");
                free(tokens);
                exit(1);
            }
            tokens = grown;
        }
        tokens[n] = lexer_next_token(&lexer);
        if (tokens[n].type == TOKEN_EOF) {
            n++;
            break;
        }
        n++;
    }

    int index_width = digit_width((int)n);
    for (size_t i = 0; i < n; i++) {
        print_token_row((int)(i + 1), index_width, &tokens[i]);
    }

    free(tokens);
}

int main(int argc, char *argv[]) {
    int print_ast = 0;
    int print_tokens = 0;
    const char *source_file = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--print-ast") == 0) {
            print_ast = 1;
        } else if (strcmp(argv[i], "--print-tokens") == 0) {
            print_tokens = 1;
        } else if (!source_file) {
            source_file = argv[i];
        } else {
            fprintf(stderr, "usage: ccompiler [--print-ast | --print-tokens] <source.c>\n");
            return 1;
        }
    }

    if (!source_file) {
        fprintf(stderr, "usage: ccompiler [--print-ast | --print-tokens] <source.c>\n");
        return 1;
    }

    /* Read source */
    char *source = read_file(source_file);

    if (print_tokens) {
        print_tokens_mode(source);
        free(source);
        return 0;
    }

    /* Lex + Parse */
    Lexer lexer;
    lexer_init(&lexer, source);

    Parser parser;
    parser_init(&parser, &lexer);

    ASTNode *ast = parse_translation_unit(&parser);

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
    codegen_translation_unit(&cg, ast);

    fclose(out);
    ast_free(ast);
    free(source);

    printf("compiled: %s -> %s\n", source_file, output_path);
    return 0;
}
