/*
 * Stage 177: unit tests for the NASM-subset assembler lexer (asm_lexer.c).
 *
 * Compile independently of the compiler binary:
 *   gcc -std=c99 -Wall -I../../include ../../src/asm_lexer.c test_asm_lexer.c -o test_asm_lexer
 *   ./test_asm_lexer
 */

#include <stdio.h>
#include <string.h>
#include "asm_lexer.h"

/* ---- minimal test framework ---- */

static int g_pass = 0;
static int g_fail = 0;

#define ASSERT(cond) do { \
    if (cond) { \
        printf("  PASS  %s\n", #cond); \
        g_pass++; \
    } else { \
        printf("  FAIL  %s  (line %d)\n", #cond, __LINE__); \
        g_fail++; \
    } \
} while (0)

static void section(const char *name) {
    printf("\n[%s]\n", name);
}

/* Collects every token from `src` (up to and including ASM_TOK_EOF) into
 * a fixed-size caller-provided array; returns the number of tokens. */
static int lex_all(const char *src, AsmToken *out, int max) {
    AsmLexer lex;
    int n = 0;
    asm_lexer_init(&lex, src);
    for (;;) {
        AsmToken tok = asm_lexer_next(&lex);
        if (n < max) out[n] = tok;
        n++;
        if (tok.type == ASM_TOK_EOF) break;
        if (n >= max) { asm_token_free(&tok); break; }
    }
    return n;
}

static void free_all(AsmToken *toks, int n) {
    int i;
    for (i = 0; i < n; i++) asm_token_free(&toks[i]);
}

/* ---- tests ---- */

static void test_instruction_line(void) {
    section("instruction line: mov rax, [rbp - 16]");
    AsmToken toks[16];
    int n = lex_all("mov rax, [rbp - 16]\n", toks, 16);

    ASSERT(n == 10);
    ASSERT(toks[0].type == ASM_TOK_IDENT && strcmp(toks[0].text, "mov") == 0);
    ASSERT(toks[1].type == ASM_TOK_IDENT && strcmp(toks[1].text, "rax") == 0);
    ASSERT(toks[2].type == ASM_TOK_COMMA);
    ASSERT(toks[3].type == ASM_TOK_LBRACKET);
    ASSERT(toks[4].type == ASM_TOK_IDENT && strcmp(toks[4].text, "rbp") == 0);
    ASSERT(toks[5].type == ASM_TOK_MINUS);
    ASSERT(toks[6].type == ASM_TOK_INT && toks[6].value == 16);
    ASSERT(toks[7].type == ASM_TOK_RBRACKET);
    ASSERT(toks[8].type == ASM_TOK_NEWLINE);
    ASSERT(toks[9].type == ASM_TOK_EOF);

    free_all(toks, n);
}

static void test_label_line(void) {
    section("label line: .L_else_0:");
    AsmToken toks[8];
    int n = lex_all(".L_else_0:\n", toks, 8);

    ASSERT(n == 4);
    ASSERT(toks[0].type == ASM_TOK_IDENT && strcmp(toks[0].text, ".L_else_0") == 0);
    ASSERT(toks[1].type == ASM_TOK_COLON);
    ASSERT(toks[2].type == ASM_TOK_NEWLINE);
    ASSERT(toks[3].type == ASM_TOK_EOF);

    free_all(toks, n);
}

static void test_hex_immediate(void) {
    section("directive line with hex immediate: mov eax, 0xff");
    AsmToken toks[8];
    int n = lex_all("mov eax, 0xff\n", toks, 8);

    ASSERT(n == 6);
    ASSERT(toks[0].type == ASM_TOK_IDENT && strcmp(toks[0].text, "mov") == 0);
    ASSERT(toks[1].type == ASM_TOK_IDENT && strcmp(toks[1].text, "eax") == 0);
    ASSERT(toks[2].type == ASM_TOK_COMMA);
    ASSERT(toks[3].type == ASM_TOK_INT && toks[3].value == 0xff);
    ASSERT(toks[4].type == ASM_TOK_NEWLINE);
    ASSERT(toks[5].type == ASM_TOK_EOF);

    free_all(toks, n);
}

static void test_comment_only_line(void) {
    section("comment-only line followed by a real line");
    AsmToken toks[16];
    int n = lex_all("; a comment\nret\n", toks, 16);

    /* The comment text itself produces no tokens; the comment line's own
     * terminating newline is still significant (ASM_TOK_NEWLINE), and the
     * following "ret\n" lexes intact. */
    ASSERT(n == 4);
    ASSERT(toks[0].type == ASM_TOK_NEWLINE);
    ASSERT(toks[1].type == ASM_TOK_IDENT && strcmp(toks[1].text, "ret") == 0);
    ASSERT(toks[2].type == ASM_TOK_NEWLINE);
    ASSERT(toks[3].type == ASM_TOK_EOF);

    free_all(toks, n);
}

/* ---- main ---- */

int main(void) {
    printf("AsmLexer unit tests\n");
    printf("====================\n");

    test_instruction_line();
    test_label_line();
    test_hex_immediate();
    test_comment_only_line();

    printf("\nResults: %d passed, %d failed, %d total\n",
           g_pass, g_fail, g_pass + g_fail);
    return g_fail > 0 ? 1 : 0;
}
