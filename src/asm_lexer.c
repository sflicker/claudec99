/*
 * asm_lexer.c - Lexer for the NASM-subset assembler (casm).
 *
 * Stage 177: recognizes identifiers (labels/mnemonics/directives/
 * registers/symbols, "." and "_" included so local labels like
 * ".L_else_0" and string labels like "Lstr3" lex as a single token),
 * decimal and 0x-hex integers, "[ ] + - , :", and newlines as explicit
 * line terminators. ";" starts a to-end-of-line comment (not observed in
 * generated output, but cheap to support). No string literals, no macro
 * or expression syntax -- not needed for the subset codegen.c emits.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "asm_lexer.h"

char *asm_strdup(const char *s) {
    size_t len;
    char  *copy;
    if (!s) return NULL;
    len = strlen(s);
    copy = malloc(len + 1);
    if (!copy) { fprintf(stderr, "casm: out of memory\n"); exit(1); }
    memcpy(copy, s, len + 1);
    return copy;
}

char *asm_read_file(const char *path) {
    FILE   *f;
    long    len;
    char   *buf;
    size_t  n;

    f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "casm: cannot open '%s'\n", path);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);

    buf = malloc((size_t)len + 1);
    if (!buf) { fprintf(stderr, "casm: out of memory\n"); exit(1); }

    n = fread(buf, 1, (size_t)len, f);
    buf[n] = '\0';
    fclose(f);
    return buf;
}

void asm_lexer_init(AsmLexer *lex, const char *src) {
    lex->src  = src;
    lex->pos  = 0;
    lex->line = 1;
    lex->col  = 1;
}

void asm_token_free(AsmToken *tok) {
    if (tok->type == ASM_TOK_IDENT) free(tok->text);
    tok->text = NULL;
}

static int is_ident_start(char c) {
    return isalpha((unsigned char)c) || c == '_' || c == '.';
}

static int is_ident_char(char c) {
    /* '-' is included so the section-attribute spelling ".note.GNU-stack"
     * (the only hyphenated identifier codegen.c ever emits) lexes as one
     * token; it never appears adjacent to a numeric literal, so this does
     * not interfere with "[rbp - 16]"-style displacement parsing, which
     * always separates the base register and '-' with whitespace. */
    return isalnum((unsigned char)c) || c == '_' || c == '.' || c == '-';
}

static char peek_char(AsmLexer *lex) { return lex->src[lex->pos]; }

static char advance_char(AsmLexer *lex) {
    char c = lex->src[lex->pos];
    if (c == '\0') return c;
    lex->pos++;
    if (c == '\n') { lex->line++; lex->col = 1; }
    else            { lex->col++; }
    return c;
}

AsmToken asm_lexer_next(AsmLexer *lex) {
    AsmToken tok;
    tok.text = NULL;
    tok.value = 0;

    for (;;) {
        char c = peek_char(lex);
        if (c == ' ' || c == '\t' || c == '\r') { advance_char(lex); continue; }
        if (c == ';') {
            while (peek_char(lex) != '\n' && peek_char(lex) != '\0')
                advance_char(lex);
            continue;
        }
        break;
    }

    tok.line = lex->line;
    tok.col  = lex->col;

    {
        char c = peek_char(lex);

        if (c == '\0') { tok.type = ASM_TOK_EOF; return tok; }

        if (c == '\n') { advance_char(lex); tok.type = ASM_TOK_NEWLINE; return tok; }

        if (isdigit((unsigned char)c)) {
            size_t start = lex->pos;
            if (c == '0' && (lex->src[lex->pos + 1] == 'x' ||
                              lex->src[lex->pos + 1] == 'X')) {
                advance_char(lex); advance_char(lex);
                while (isxdigit((unsigned char)peek_char(lex))) advance_char(lex);
                tok.value = strtol(lex->src + start, NULL, 16);
            } else {
                while (isdigit((unsigned char)peek_char(lex))) advance_char(lex);
                tok.value = strtol(lex->src + start, NULL, 10);
            }
            tok.type = ASM_TOK_INT;
            return tok;
        }

        if (is_ident_start(c)) {
            size_t start = lex->pos;
            size_t len;
            while (is_ident_char(peek_char(lex))) advance_char(lex);
            len = lex->pos - start;
            tok.text = malloc(len + 1);
            if (!tok.text) { fprintf(stderr, "casm: out of memory\n"); exit(1); }
            memcpy(tok.text, lex->src + start, len);
            tok.text[len] = '\0';
            tok.type = ASM_TOK_IDENT;
            return tok;
        }

        advance_char(lex);
        switch (c) {
        case '[': tok.type = ASM_TOK_LBRACKET; return tok;
        case ']': tok.type = ASM_TOK_RBRACKET; return tok;
        case '+': tok.type = ASM_TOK_PLUS;     return tok;
        case '-': tok.type = ASM_TOK_MINUS;    return tok;
        case ',': tok.type = ASM_TOK_COMMA;    return tok;
        case ':': tok.type = ASM_TOK_COLON;    return tok;
        default:  tok.type = ASM_TOK_UNKNOWN;  return tok;
        }
    }
}
