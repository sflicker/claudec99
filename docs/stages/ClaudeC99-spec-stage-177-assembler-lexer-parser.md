# ClaudeC99 Stage 177 — Assembler: Lexer and Parser Infrastructure

## Goal

Begin the custom assembler (`casm`) that will eventually replace the external
`nasm` dependency: a standalone command-line tool that reads a `.asm` file in
the exact NASM-syntax subset `src/codegen.c` emits and parses it into an array
of structured `AsmLine` records.

This stage adds **lexer + parser infrastructure only** — no encoding, no
relocations, no ELF object output.  `casm`, when run, parses its input file
and (with `--dump`) prints a normalized textual form of every parsed line;
otherwise it reports a line count and exits 0.  This proves the front end is
complete and correct against real compiler output before any bytes are
encoded.  Subsequent stages add the symbol table, encoder, and ELF64 writer
on top of this skeleton (see `docs/outlines/checklist.md`, "Assembler"
section).

`casm` is a **new, independent executable** — it does not touch
`src/compiler.c`, `src/codegen.c`, or any existing ccompiler source, and it is
not yet wired into `build.sh` or `bin/cc99`.  Nothing about the existing
compile pipeline changes in this stage.

---

## Background — scope of the NASM subset

The full assembler design (`docs/outlines/checklist.md`, "Assembler" section)
was scoped by measuring every `.asm` file produced by the compiler's own
self-hosted build (`build/**/*.asm`, ~259K lines across three bootstrap
generations). The lexer and parser built in this stage must accept 100% of
that corpus. Measured grammar:

- **Lines** are one of: a bare label (`name:`, including local labels like
  `.L_else_0:` and generated string labels like `Lstr3:`), a directive line
  (`section .text`, `global add`, `extern printf`, `db 1, 2, 3`,
  `resb 128`), or an instruction line (`mov rax, [rbp - 16]`).
- **No comments, no quoted string literals, no macros, no expressions**
  appear anywhere in generated output (confirmed by corpus grep — `;`, `"`,
  and multiplication/addition in operand position never occur). The lexer
  tolerates `;`-to-end-of-line comments for robustness but the parser does
  not need to handle string-literal `db` operands or symbolic arithmetic.
- **Operand forms**: bare registers, decimal/`0x`-hex immediates (optionally
  negative), and exactly four memory forms — `[reg]`, `[rbp ± disp]`,
  `[rsp + disp]`, `[rel symbol]`. No scaled-index/SIB addressing is ever
  emitted.
- **Directive argument styles** differ by directive: `db`/`dd`/`dq` take a
  **comma-separated** value list; `section`/`global`/`extern`/`resb`/etc.
  take a **space-separated** bare token list (e.g.
  `section .note.GNU-stack noalloc noexec nowrite progbits` has five
  space-separated arguments, no commas).

---

## Architecture

```
casm <file.asm> [--dump]
        │
        ▼
  read_whole_file()
        │
        ▼
  AsmLexer ──asm_lexer_next()──▶ AsmToken stream
        │
        ▼
  AsmParser (asm_parser_parse) ──▶ Vec<AsmLine>
        │
        ▼
  --dump: asm_dump_line() per entry, else just report count
```

`casm` is built as a second CMake executable target alongside `ccompiler`.
It links `src/vec.c` (the existing generic growable-array module — reused
here rather than hand-rolling another dynamic array, per the project's
stage-92 convention of avoiding fixed/static arrays) but **does not** link
`src/util.c`: that module's `compile_error`/`compile_warning_at` machinery is
built around ccompiler's global error-recovery state (`g_error_jmp`,
`g_warn_level`, etc.), which has no meaning for a standalone assembler.
`casm`'s own small file-reading and string-duplication helpers live in
`src/asm_lexer.c` instead, keeping the new module decoupled from the
compiler's CLI globals.

---

## Task 1 — `include/asm_lexer.h`

```c
#ifndef CCOMPILER_ASM_LEXER_H
#define CCOMPILER_ASM_LEXER_H

#include <stddef.h>

/* Stage 177: token kinds for the NASM-subset assembler lexer. Comments
 * (";" to end of line) and whitespace are consumed silently and never
 * produce a token. Newlines ARE significant: each source line becomes
 * exactly one AsmLine in the parser, so ASM_TOK_NEWLINE is the line
 * terminator the parser scans for. */
typedef enum {
    ASM_TOK_EOF,
    ASM_TOK_NEWLINE,
    ASM_TOK_IDENT,      /* label / mnemonic / directive / register / symbol */
    ASM_TOK_INT,        /* decimal or 0x-hex integer literal (unsigned
                          * magnitude; a leading "-" lexes as ASM_TOK_MINUS
                          * and is folded into the value by the parser) */
    ASM_TOK_LBRACKET,
    ASM_TOK_RBRACKET,
    ASM_TOK_PLUS,
    ASM_TOK_MINUS,
    ASM_TOK_COMMA,
    ASM_TOK_COLON,
    ASM_TOK_UNKNOWN
} AsmTokenType;

typedef struct {
    AsmTokenType type;
    char        *text;   /* heap-owned spelling; only set for ASM_TOK_IDENT */
    long         value;  /* decoded value; only set for ASM_TOK_INT */
    int          line;   /* 1-based source line */
    int          col;    /* 1-based source column */
} AsmToken;

typedef struct {
    const char *src;   /* full source buffer, not owned by the lexer */
    size_t      pos;
    int         line;
    int         col;
} AsmLexer;

void     asm_lexer_init(AsmLexer *lex, const char *src);
AsmToken asm_lexer_next(AsmLexer *lex);
void     asm_token_free(AsmToken *tok);

/* Small standalone helpers, used by casm.c and asm_parser.c. Deliberately
 * not shared with src/util.c -- see the Architecture note in
 * docs/stages/ClaudeC99-spec-stage-177-assembler-lexer-parser.md. */
char *asm_read_file(const char *path);
char *asm_strdup(const char *s);

#endif /* CCOMPILER_ASM_LEXER_H */
```

---

## Task 2 — `src/asm_lexer.c`

```c
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
    return isalnum((unsigned char)c) || c == '_' || c == '.';
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
```

---

## Task 3 — `include/asm_parser.h`

```c
#ifndef CCOMPILER_ASM_PARSER_H
#define CCOMPILER_ASM_PARSER_H

#include "vec.h"

typedef enum {
    ASM_LINE_LABEL,
    ASM_LINE_DIRECTIVE,
    ASM_LINE_INSTRUCTION
} AsmLineKind;

typedef enum {
    ASM_OPERAND_REG,
    ASM_OPERAND_IMM,
    ASM_OPERAND_MEM,
    ASM_OPERAND_SYM
} AsmOperandKind;

/* Stage 177: flat operand record (mirrors the ASTNode convention of one
 * struct with kind-specific fields documented per field, rather than a
 * tagged union) covering every operand form observed in generated NASM
 * output: bare register, immediate, one of four memory forms, or a bare
 * symbol (call/jmp target, dd/dq data value, directive argument). */
typedef struct {
    AsmOperandKind kind;

    char  reg[8];        /* ASM_OPERAND_REG: register name, e.g. "rax",
                           * "r15d", "xmm3", "dil" */

    long  imm;           /* ASM_OPERAND_IMM: signed immediate value */

    char *sym;            /* ASM_OPERAND_SYM: heap-owned bare symbol name */

    int   mem_is_rip;     /* ASM_OPERAND_MEM: 1 for "[rel sym]" form */
    char  mem_base[8];    /* ASM_OPERAND_MEM, mem_is_rip==0: base register,
                            * e.g. "rbp", "rax", "rsp" */
    int   mem_has_disp;   /* ASM_OPERAND_MEM, mem_is_rip==0: 1 if a
                            * "+ N" / "- N" displacement follows the base */
    long  mem_disp;       /* ASM_OPERAND_MEM, mem_is_rip==0: signed
                            * displacement value (0 if mem_has_disp == 0) */
    char *mem_sym;        /* ASM_OPERAND_MEM, mem_is_rip==1: heap-owned
                            * symbol name inside "[rel ...]" */
    const char *size_hint; /* ASM_OPERAND_MEM only: "byte"/"word"/"dword",
                             * or NULL; points to a static string literal,
                             * not owned */
} AsmOperand;

typedef struct {
    AsmLineKind kind;
    int         line_no;      /* 1-based source line, for diagnostics */

    char       *label;        /* ASM_LINE_LABEL: heap-owned label name */

    char       *directive;    /* ASM_LINE_DIRECTIVE: heap-owned directive
                                * name ("section", "global", "extern",
                                * "db", "dd", "dq", "resb", ...) */
    Vec         directive_args; /* Vec<AsmOperand>: comma-separated value
                                  * list for db/dd/dq; space-separated bare
                                  * token list for every other directive */

    char       *mnemonic;     /* ASM_LINE_INSTRUCTION: heap-owned mnemonic,
                                * lowercase as written ("mov", "je", ...) */
    Vec         operands;     /* ASM_LINE_INSTRUCTION: Vec<AsmOperand>,
                                * in source order (0 to 3 operands observed) */
} AsmLine;

/*
 * asm_parser_parse - parse the full contents of `src` (already read into
 * memory) into a flat sequence of AsmLine records appended to *out_lines.
 * `filename` is used only for diagnostic messages.
 *
 * *out_lines must already be vec_init'd with sizeof(AsmLine).
 *
 * On a syntax error, prints "filename:line: asm parse error: message" to
 * stderr and calls exit(1) -- casm has no error-recovery mode; a malformed
 * input file (which should never occur for compiler-generated output) is
 * always fatal.
 */
void asm_parser_parse(const char *src, const char *filename, Vec *out_lines);

/* asm_line_free - release the heap-owned fields of one AsmLine (does not
 * free the AsmLine struct itself, since callers store these by value in
 * a Vec). */
void asm_line_free(AsmLine *line);

/* asm_dump_line - print a normalized one-line textual form of `line` to
 * `out`, for --dump and for the unit tests to assert against. */
void asm_dump_line(FILE *out, const AsmLine *line);

#endif /* CCOMPILER_ASM_PARSER_H */
```

---

## Task 4 — `src/asm_parser.c`

Includes: `<stdio.h>`, `<stdlib.h>`, `<string.h>`, `"asm_parser.h"`,
`"asm_lexer.h"`.

Implementation notes (write the full file from these):

### 4a — Register name table

A static array of every register name observed (plus the systematic
completions of the same families) is used only to classify a bare
`ASM_TOK_IDENT` operand as `ASM_OPERAND_REG` vs. `ASM_OPERAND_SYM` — the
base register inside `[...]` brackets does not need this check, since
anything in base-register position is captured as-is.

```c
static const char *ASM_REGISTERS[] = {
    /* 64-bit */
    "rax","rbx","rcx","rdx","rsi","rdi","rbp","rsp",
    "r8","r9","r10","r11","r12","r13","r14","r15","rip",
    /* 32-bit */
    "eax","ebx","ecx","edx","esi","edi","ebp","esp",
    "r8d","r9d","r10d","r11d","r12d","r13d","r14d","r15d",
    /* 16-bit */
    "ax","bx","cx","dx","si","di","bp","sp",
    "r8w","r9w","r10w","r11w","r12w","r13w","r14w","r15w",
    /* 8-bit */
    "al","bl","cl","dl","sil","dil","bpl","spl",
    "ah","bh","ch","dh",
    "r8b","r9b","r10b","r11b","r12b","r13b","r14b","r15b",
    /* SSE */
    "xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7",
    "xmm8","xmm9","xmm10","xmm11","xmm12","xmm13","xmm14","xmm15",
    NULL
};

static int asm_is_register(const char *name) {
    int i;
    for (i = 0; ASM_REGISTERS[i]; i++)
        if (strcmp(ASM_REGISTERS[i], name) == 0) return 1;
    return 0;
}
```

### 4b — Directive classification

```c
static int is_data_directive(const char *name) {
    return strcmp(name, "db") == 0 || strcmp(name, "dd") == 0 ||
           strcmp(name, "dq") == 0;
}

static int is_known_directive(const char *name) {
    static const char *names[] = {
        "section", "global", "extern", "db", "dd", "dq", "resb",
        "bits", "default", NULL
    };
    int i;
    for (i = 0; names[i]; i++)
        if (strcmp(names[i], name) == 0) return 1;
    return 0;
}
```

### 4c — Parser state and error helper

```c
typedef struct {
    AsmLexer    lex;
    AsmToken    tok;      /* current lookahead token */
    const char *filename;
} AsmParser;

static void asm_advance(AsmParser *p) {
    asm_token_free(&p->tok);
    p->tok = asm_lexer_next(&p->lex);
}

static void asm_parse_error(AsmParser *p, const char *msg) {
    fprintf(stderr, "%s:%d: asm parse error: %s\n",
            p->filename, p->tok.line, msg);
    exit(1);
}
```

### 4d — Operand parsing

One shared routine for instruction operands (registers, immediates, all
four memory forms), plus a restricted routine for `db`/`dd`/`dq` value
lists (integers and bare symbols only — never brackets or registers, per
the corpus).

```c
static void parse_mem_operand(AsmParser *p, AsmOperand *op,
                              const char *size_hint) {
    op->kind = ASM_OPERAND_MEM;
    op->size_hint = size_hint;
    op->mem_is_rip = 0;
    op->mem_has_disp = 0;
    op->mem_disp = 0;
    op->mem_sym = NULL;
    op->mem_base[0] = '\0';

    asm_advance(p); /* consume '[' */

    if (p->tok.type == ASM_TOK_IDENT && strcmp(p->tok.text, "rel") == 0) {
        asm_advance(p); /* consume "rel" */
        if (p->tok.type != ASM_TOK_IDENT)
            asm_parse_error(p, "expected symbol after 'rel'");
        op->mem_is_rip = 1;
        op->mem_sym = asm_strdup(p->tok.text);
        asm_advance(p);
    } else {
        if (p->tok.type != ASM_TOK_IDENT)
            asm_parse_error(p, "expected base register in memory operand");
        strncpy(op->mem_base, p->tok.text, sizeof(op->mem_base) - 1);
        op->mem_base[sizeof(op->mem_base) - 1] = '\0';
        asm_advance(p);

        if (p->tok.type == ASM_TOK_PLUS || p->tok.type == ASM_TOK_MINUS) {
            int negative = (p->tok.type == ASM_TOK_MINUS);
            asm_advance(p);
            if (p->tok.type != ASM_TOK_INT)
                asm_parse_error(p, "expected displacement after +/-");
            op->mem_has_disp = 1;
            op->mem_disp = negative ? -p->tok.value : p->tok.value;
            asm_advance(p);
        }
    }

    if (p->tok.type != ASM_TOK_RBRACKET)
        asm_parse_error(p, "expected ']'");
    asm_advance(p);
}

static void parse_operand(AsmParser *p, AsmOperand *op) {
    if (p->tok.type == ASM_TOK_IDENT &&
        (strcmp(p->tok.text, "byte") == 0 ||
         strcmp(p->tok.text, "word") == 0 ||
         strcmp(p->tok.text, "dword") == 0)) {
        static const char *hints[] = { "byte", "word", "dword" };
        const char *hint = NULL;
        int i;
        for (i = 0; i < 3; i++)
            if (strcmp(p->tok.text, hints[i]) == 0) hint = hints[i];
        asm_advance(p);
        if (p->tok.type != ASM_TOK_LBRACKET)
            asm_parse_error(p, "expected '[' after size hint");
        parse_mem_operand(p, op, hint);
        return;
    }

    if (p->tok.type == ASM_TOK_LBRACKET) {
        parse_mem_operand(p, op, NULL);
        return;
    }

    if (p->tok.type == ASM_TOK_MINUS) {
        asm_advance(p);
        if (p->tok.type != ASM_TOK_INT)
            asm_parse_error(p, "expected integer after unary '-'");
        op->kind = ASM_OPERAND_IMM;
        op->imm = -p->tok.value;
        asm_advance(p);
        return;
    }

    if (p->tok.type == ASM_TOK_INT) {
        op->kind = ASM_OPERAND_IMM;
        op->imm = p->tok.value;
        asm_advance(p);
        return;
    }

    if (p->tok.type == ASM_TOK_IDENT) {
        if (asm_is_register(p->tok.text)) {
            op->kind = ASM_OPERAND_REG;
            strncpy(op->reg, p->tok.text, sizeof(op->reg) - 1);
            op->reg[sizeof(op->reg) - 1] = '\0';
        } else {
            op->kind = ASM_OPERAND_SYM;
            op->sym = asm_strdup(p->tok.text);
        }
        asm_advance(p);
        return;
    }

    asm_parse_error(p, "expected an operand");
}

/* db/dd/dq values: integer (optionally negative) or a bare symbol only. */
static void parse_data_value(AsmParser *p, AsmOperand *op) {
    if (p->tok.type == ASM_TOK_MINUS) {
        asm_advance(p);
        if (p->tok.type != ASM_TOK_INT)
            asm_parse_error(p, "expected integer after unary '-'");
        op->kind = ASM_OPERAND_IMM;
        op->imm = -p->tok.value;
        asm_advance(p);
    } else if (p->tok.type == ASM_TOK_INT) {
        op->kind = ASM_OPERAND_IMM;
        op->imm = p->tok.value;
        asm_advance(p);
    } else if (p->tok.type == ASM_TOK_IDENT) {
        op->kind = ASM_OPERAND_SYM;
        op->sym = asm_strdup(p->tok.text);
        asm_advance(p);
    } else {
        asm_parse_error(p, "expected a data value (integer or symbol)");
    }
}

/* Bare token argument for non-data directives (section names, global/
 * extern symbol names, section-attribute flags, resb's numeric size). */
static void parse_bare_arg(AsmParser *p, AsmOperand *op) {
    if (p->tok.type == ASM_TOK_INT) {
        op->kind = ASM_OPERAND_IMM;
        op->imm = p->tok.value;
        asm_advance(p);
    } else if (p->tok.type == ASM_TOK_IDENT) {
        op->kind = ASM_OPERAND_SYM;
        op->sym = asm_strdup(p->tok.text);
        asm_advance(p);
    } else {
        asm_parse_error(p, "expected a directive argument");
    }
}
```

### 4e — Line-level parsing and the driver loop

```c
static void expect_line_end(AsmParser *p) {
    if (p->tok.type != ASM_TOK_NEWLINE && p->tok.type != ASM_TOK_EOF)
        asm_parse_error(p, "expected end of line");
    if (p->tok.type == ASM_TOK_NEWLINE)
        asm_advance(p);
}

void asm_parser_parse(const char *src, const char *filename, Vec *out_lines) {
    AsmParser p;
    p.filename = filename;
    asm_lexer_init(&p.lex, src);
    p.tok = asm_lexer_next(&p.lex);

    while (p.tok.type != ASM_TOK_EOF) {
        AsmLine line;

        if (p.tok.type == ASM_TOK_NEWLINE) { asm_advance(&p); continue; }

        if (p.tok.type != ASM_TOK_IDENT)
            asm_parse_error(&p, "expected a label, directive, or mnemonic");

        line.line_no = p.tok.line;
        line.label = NULL;
        line.directive = NULL;
        line.mnemonic = NULL;

        {
            char *name = asm_strdup(p.tok.text);
            asm_advance(&p);

            if (p.tok.type == ASM_TOK_COLON) {
                asm_advance(&p);
                line.kind = ASM_LINE_LABEL;
                line.label = name;
                expect_line_end(&p);
            } else if (is_known_directive(name)) {
                line.kind = ASM_LINE_DIRECTIVE;
                line.directive = name;
                vec_init(&line.directive_args, sizeof(AsmOperand));
                if (p.tok.type != ASM_TOK_NEWLINE && p.tok.type != ASM_TOK_EOF) {
                    if (is_data_directive(name)) {
                        for (;;) {
                            AsmOperand op;
                            parse_data_value(&p, &op);
                            vec_push(&line.directive_args, &op);
                            if (p.tok.type != ASM_TOK_COMMA) break;
                            asm_advance(&p);
                        }
                    } else {
                        while (p.tok.type == ASM_TOK_IDENT ||
                               p.tok.type == ASM_TOK_INT) {
                            AsmOperand op;
                            parse_bare_arg(&p, &op);
                            vec_push(&line.directive_args, &op);
                        }
                    }
                }
                expect_line_end(&p);
            } else {
                line.kind = ASM_LINE_INSTRUCTION;
                line.mnemonic = name;
                vec_init(&line.operands, sizeof(AsmOperand));
                if (p.tok.type != ASM_TOK_NEWLINE && p.tok.type != ASM_TOK_EOF) {
                    for (;;) {
                        AsmOperand op;
                        parse_operand(&p, &op);
                        vec_push(&line.operands, &op);
                        if (p.tok.type != ASM_TOK_COMMA) break;
                        asm_advance(&p);
                    }
                }
                expect_line_end(&p);
            }
        }

        vec_push(out_lines, &line);
    }
}
```

`asm_line_free` frees `label`/`directive`/`mnemonic`, walks `directive_args`
/`operands` freeing each `AsmOperand`'s `sym`/`mem_sym`, then calls
`vec_free` on whichever `Vec` the line kind populated.

`asm_dump_line` prints one normalized line per record, e.g.:

```
LABEL       .L_else_0
DIRECTIVE   section .text
DIRECTIVE   global add
DIRECTIVE   db 84 79 75 69 78 95 69 79 70 0
INSTR       mov     REG:rax, MEM(base=rbp disp=-16)
INSTR       lea     REG:rax, MEMREL(sym=Lstr0)
INSTR       call    SYM:add
INSTR       ret
```

(Exact column widths/spacing are an implementation choice — the unit tests
below assert on the field content, not byte-exact formatting.)

---

## Task 5 — `src/casm.c`

```c
/*
 * casm.c - Entry point for the NASM-subset assembler.
 *
 * Stage 177: parses the input file and either dumps the parsed AsmLine
 * records (--dump) or reports a line count. No encoding or object-file
 * output yet -- see docs/outlines/checklist.md, "Assembler" section, for
 * the milestones that follow.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "asm_lexer.h"
#include "asm_parser.h"
#include "vec.h"

static void usage(const char *prog) {
    fprintf(stderr, "usage: %s [--dump] <file.asm>\n", prog);
}

int main(int argc, char **argv) {
    const char *path = NULL;
    int dump = 0;
    int i;
    char *src;
    Vec lines;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--dump") == 0) dump = 1;
        else if (path == NULL) path = argv[i];
        else { usage(argv[0]); return 1; }
    }
    if (!path) { usage(argv[0]); return 1; }

    src = asm_read_file(path);
    vec_init(&lines, sizeof(AsmLine));
    asm_parser_parse(src, path, &lines);

    if (dump) {
        size_t j;
        for (j = 0; j < lines.len; j++)
            asm_dump_line(stdout, (AsmLine *)vec_get(&lines, j));
    } else {
        printf("casm: parsed %zu lines from %s\n", lines.len, path);
    }

    {
        size_t j;
        for (j = 0; j < lines.len; j++)
            asm_line_free((AsmLine *)vec_get(&lines, j));
    }
    vec_free(&lines);
    free(src);
    return 0;
}
```

---

## Task 6 — `CMakeLists.txt`

Add a second executable target after the existing `ccompiler` target:

```cmake
add_executable(casm
    src/casm.c
    src/asm_lexer.c
    src/asm_parser.c
    src/vec.c
)
target_include_directories(casm PRIVATE include)
target_compile_options(casm PRIVATE -Wall -Wextra -pedantic)

add_custom_command(TARGET casm POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:casm> ${CMAKE_SOURCE_DIR}/bin/casm
)
```

`casm` does not need `VERSION_BUILD`/`VERSION_BUILTBY` definitions — those
are specific to `ccompiler --version` output and have no equivalent here
yet.

---

## Bootstrap caveats

`casm` is **not** part of the C0→C1→C2 self-hosting chain at this stage —
it is built only with the system compiler (via CMake), the same way the
`test/unit/*.c` harnesses are. `./build.sh --mode=self-host` is unaffected
by this stage and must still pass unchanged; do not add the new `src/asm_*.c`
/ `src/casm.c` files to `build.sh`'s `SOURCES` list or to `ccompiler`'s
CMake source list.

Note for future stages: `src/*.c` glob-based tooling (the
`self-compile-report` skill) will pick up `asm_lexer.c`/`asm_parser.c`/
`casm.c` automatically and attempt to compile them with the project's own
`ccompiler`. That is informational only — it is not a pass/fail gate for
this stage — but keeping the new files free of gratuitous GNU extensions
(as done above: no `//` comments in the reference code, declarations
grouped C89-style at block start where practical) keeps the door open for
`casm` to eventually self-host too, mirroring the rest of the project.

---

## Tests

### Unit tests

Add two new unit tests to `test/unit/`, following the existing harness
convention in `test/unit/run_tests.sh` (a `main()` that runs assertions and
prints a `Results: P passed, F failed, T total` summary line).

**`test/unit/test_asm_lexer.c`** — feed short hand-written strings directly
to `asm_lexer_next()` in a loop and assert the resulting token sequence
(type, and `text`/`value` where relevant) for:
- an instruction line: `"mov rax, [rbp - 16]\n"`
- a label line: `".L_else_0:\n"`
- a directive line with a hex immediate: `"mov eax, 0xff\n"`
- a `;`-comment-only line followed by a real line, verifying the comment
  produces no tokens and the following `ASM_TOK_NEWLINE`/content is intact

**`test/unit/test_asm_parser.c`** — feed short hand-written `.asm` snippets
to `asm_parser_parse()` and assert on the resulting `AsmLine` fields (not
the dump text) for one example of each case in the corpus:
- a bare label (`"foo:\n"` → `ASM_LINE_LABEL`, `label == "foo"`)
- a `section`/`global`/`extern` directive (space-separated bare args)
- a `db` directive with a comma-separated integer list
- a `dq` directive with a symbol operand
- `[rbp - N]`, `[rsp + N]`, `[rax]`, and `[rel sym]` memory operands
- a negative and a `0x`-hex immediate operand
- a zero-operand instruction (`"ret\n"`, `"cqo\n"`)
- a three-operand instruction (`"imul rax, rax, 4\n"`)

Register both in `test/unit/run_tests.sh`:

```bash
run_unit_test "test_asm_lexer" \
    "${SCRIPT_DIR}/test_asm_lexer.c" \
    "${PROJECT_DIR}/src/asm_lexer.c"

run_unit_test "test_asm_parser" \
    "${SCRIPT_DIR}/test_asm_parser.c" \
    "${PROJECT_DIR}/src/asm_lexer.c ${PROJECT_DIR}/src/asm_parser.c ${PROJECT_DIR}/src/vec.c"
```

### Corpus smoke test

Add `test/asm/run_casm_smoke.sh`, matching the project's per-category
`run_tests.sh` convention (see `test/unit/run_tests.sh`,
`test/build_tool/run_tests.sh`): it globs every `*.asm` file under
`build/`, runs `${PROJECT_DIR}/build/casm "$f"` on each, and reports a
`Results: P passed, F failed, T total` summary line (pass = exit code 0).

```bash
#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
CASM="${PROJECT_DIR}/build/casm"

pass=0
fail=0
if [ ! -x "$CASM" ]; then
    echo "SKIP  test/asm: build/casm not built (run cmake --build build)"
    echo "Results: 0 passed, 0 failed, 0 total"
    exit 0
fi

shopt -s globstar nullglob
files=("${PROJECT_DIR}"/build/**/*.asm)
if [ ${#files[@]} -eq 0 ]; then
    echo "SKIP  test/asm: no *.asm files under build/ (run ./build.sh --mode=self-host first)"
    echo "Results: 0 passed, 0 failed, 0 total"
    exit 0
fi

for f in "${files[@]}"; do
    if "$CASM" "$f" > /dev/null 2>&1; then
        pass=$((pass + 1))
    else
        echo "FAIL  asm/$(basename "$f")"
        fail=$((fail + 1))
    fi
done

echo "Results: ${pass} passed, ${fail} failed, $((pass + fail)) total"
[ "$fail" -eq 0 ]
```

This is the front-end half of the differential-testing plan in the
checklist's Assembler section (Milestone 1: "parse every file under
`build/**/*.asm` without error"). Byte-level comparison against `nasm -f
elf64` output is out of scope until the encoder exists (later stages).

Run it after a self-hosted build has populated `build/`:

```sh
cmake --build build
./build.sh --mode=self-host
./test/asm/run_casm_smoke.sh
```

---

## Implementation order

1. `include/asm_lexer.h` — token types, `AsmLexer`, `AsmToken`, and the two
   standalone helper declarations.
2. `src/asm_lexer.c` — `asm_strdup`, `asm_read_file`, `asm_lexer_init`,
   `asm_lexer_next`, `asm_token_free`.
3. `include/asm_parser.h` — `AsmOperand`, `AsmLine`, and the three public
   functions.
4. `src/asm_parser.c` — register table, directive classification, operand
   parsers, line parser, `asm_parser_parse`, `asm_line_free`,
   `asm_dump_line`.
5. `src/casm.c` — driver `main()`.
6. `CMakeLists.txt` — add the `casm` target (Task 6).
7. Build: `cmake --build build`. Smoke test by hand:
   ```sh
   printf 'section .text\nglobal main\nmain:\n    mov eax, 0\n    ret\n' > /tmp/t.asm
   ./build/casm --dump /tmp/t.asm
   ```
8. Add `test/unit/test_asm_lexer.c` and `test/unit/test_asm_parser.c`;
   wire both into `test/unit/run_tests.sh`; run
   `./test/unit/run_tests.sh` — all pass.
9. Add `test/asm/run_casm_smoke.sh`; run
   `./build.sh --mode=self-host` (unaffected, must still pass) then
   `./test/asm/run_casm_smoke.sh` — 0 failures across the full corpus.
10. `src/version.c` — bump `VERSION_STAGE` to `"01770000"`.

---

## Out of scope — do NOT do these in this stage

- **Symbol table** (`asm_symtab.h`/`.c`) and **relocations**
  (`asm_reloc.h`/`.c`) — later stages.
- **Instruction encoding** (`asm_encode.h`/`.c`, REX/ModRM/SIB, opcode
  tables) — later stages.
- **ELF64 object writing** (`elf_writer.h`/`.c`) — later stages.
- **Two-pass layout/offset assignment** — this stage only produces a flat
  `AsmLine` list; section-offset bookkeeping starts once the encoder exists.
- **Wiring `casm` into `build.sh`, `bin/cc99`, or replacing the `nasm`
  invocation anywhere** — `nasm` remains the only assembler actually used
  to produce runnable binaries until `casm` reaches ELF-object parity
  (checklist Milestone 4+).
- **Differential testing against `nasm -f elf64` byte output** — meaningless
  before an encoder exists; this stage's regression is parse-success only.
- **Scaled-index/SIB addressing, 16-bit operand-size instructions, string
  literal `db` operands, NASM macros/preprocessor (`%define`, `%macro`,
  `%if`)** — never emitted by `codegen.c`; not part of the target subset at
  all (see `docs/outlines/checklist.md`).

---

## Docs (at stage close, after all tests pass)

- **`docs/outlines/checklist.md`** — mark the lexer/parser/operand-model
  infrastructure items and Milestone 1 under the "Assembler" section as
  complete (`[x]`), annotating Stage 177; add a `## Stage 177` section after
  `## Stage 176`.
- Run the **`update-supplemental-docs`** skill to refresh snapshots.
- **`docs/self-compilation-report.md`** — no self-host run is required for
  this stage (see Bootstrap caveats), so no update needed unless the
  `self-compile-report` skill is separately re-run out of curiosity.
- Update **`src/version.c`** (`VERSION_STAGE "01770000"`).
