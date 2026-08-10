#ifndef CCOMPILER_ASM_PARSER_H
#define CCOMPILER_ASM_PARSER_H

#include <stdio.h>
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
