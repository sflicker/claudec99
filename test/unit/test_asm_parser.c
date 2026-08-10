/*
 * Stage 177: unit tests for the NASM-subset assembler parser (asm_parser.c).
 *
 * Compile independently of the compiler binary:
 *   gcc -std=c99 -Wall -I../../include \
 *       ../../src/asm_lexer.c ../../src/asm_parser.c ../../src/vec.c \
 *       test_asm_parser.c -o test_asm_parser
 *   ./test_asm_parser
 */

#include <stdio.h>
#include <string.h>
#include "asm_lexer.h"
#include "asm_parser.h"
#include "vec.h"

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

static void parse_src(const char *src, Vec *out) {
    vec_init(out, sizeof(AsmLine));
    asm_parser_parse(src, "<test>", out);
}

static void free_lines(Vec *v) {
    size_t i;
    for (i = 0; i < v->len; i++)
        asm_line_free((AsmLine *)vec_get(v, i));
    vec_free(v);
}

/* ---- tests ---- */

static void test_bare_label(void) {
    section("bare label: foo:");
    Vec lines;
    parse_src("foo:\n", &lines);

    ASSERT(lines.len == 1);
    AsmLine *l = (AsmLine *)vec_get(&lines, 0);
    ASSERT(l->kind == ASM_LINE_LABEL);
    ASSERT(strcmp(l->label, "foo") == 0);

    free_lines(&lines);
}

static void test_section_global_extern(void) {
    section("section / global / extern directives (space-separated args)");
    Vec lines;
    parse_src("section .text\nglobal add\nextern printf\n", &lines);

    ASSERT(lines.len == 3);

    AsmLine *l0 = (AsmLine *)vec_get(&lines, 0);
    ASSERT(l0->kind == ASM_LINE_DIRECTIVE);
    ASSERT(strcmp(l0->directive, "section") == 0);
    ASSERT(l0->directive_args.len == 1);
    AsmOperand *a0 = (AsmOperand *)vec_get(&l0->directive_args, 0);
    ASSERT(a0->kind == ASM_OPERAND_SYM && strcmp(a0->sym, ".text") == 0);

    AsmLine *l1 = (AsmLine *)vec_get(&lines, 1);
    ASSERT(strcmp(l1->directive, "global") == 0);
    ASSERT(l1->directive_args.len == 1);
    AsmOperand *a1 = (AsmOperand *)vec_get(&l1->directive_args, 0);
    ASSERT(strcmp(a1->sym, "add") == 0);

    AsmLine *l2 = (AsmLine *)vec_get(&lines, 2);
    ASSERT(strcmp(l2->directive, "extern") == 0);
    AsmOperand *a2 = (AsmOperand *)vec_get(&l2->directive_args, 0);
    ASSERT(strcmp(a2->sym, "printf") == 0);

    free_lines(&lines);
}

static void test_db_comma_list(void) {
    section("db directive with comma-separated integer list");
    Vec lines;
    parse_src("db 1, 2, 3\n", &lines);

    ASSERT(lines.len == 1);
    AsmLine *l = (AsmLine *)vec_get(&lines, 0);
    ASSERT(l->kind == ASM_LINE_DIRECTIVE);
    ASSERT(strcmp(l->directive, "db") == 0);
    ASSERT(l->directive_args.len == 3);
    ASSERT(((AsmOperand *)vec_get(&l->directive_args, 0))->imm == 1);
    ASSERT(((AsmOperand *)vec_get(&l->directive_args, 1))->imm == 2);
    ASSERT(((AsmOperand *)vec_get(&l->directive_args, 2))->imm == 3);

    free_lines(&lines);
}

static void test_dq_symbol(void) {
    section("dq directive with a symbol operand");
    Vec lines;
    parse_src("dq some_func\n", &lines);

    ASSERT(lines.len == 1);
    AsmLine *l = (AsmLine *)vec_get(&lines, 0);
    ASSERT(strcmp(l->directive, "dq") == 0);
    ASSERT(l->directive_args.len == 1);
    AsmOperand *op = (AsmOperand *)vec_get(&l->directive_args, 0);
    ASSERT(op->kind == ASM_OPERAND_SYM && strcmp(op->sym, "some_func") == 0);

    free_lines(&lines);
}

static void test_label_attached_directive(void) {
    section("label-attached data directive: Lstr0: db 72, 0");
    Vec lines;
    parse_src("Lstr0: db 72, 0\n", &lines);

    /* Real codegen.c output attaches db/dd/dq/resb directives to their
     * label on the same source line; the parser splits this into two
     * AsmLine records sharing the same line number. */
    ASSERT(lines.len == 2);
    AsmLine *l0 = (AsmLine *)vec_get(&lines, 0);
    ASSERT(l0->kind == ASM_LINE_LABEL);
    ASSERT(strcmp(l0->label, "Lstr0") == 0);
    AsmLine *l1 = (AsmLine *)vec_get(&lines, 1);
    ASSERT(l1->kind == ASM_LINE_DIRECTIVE);
    ASSERT(strcmp(l1->directive, "db") == 0);
    ASSERT(l1->directive_args.len == 2);
    ASSERT(l1->line_no == l0->line_no);

    free_lines(&lines);
}

static void test_mem_operands(void) {
    section("memory operand forms: [rbp-N] [rsp+N] [rax] [rel sym]");
    Vec lines;
    parse_src(
        "mov rax, [rbp - 16]\n"
        "mov rax, [rsp + 8]\n"
        "mov rax, [rax]\n"
        "lea rax, [rel Lstr0]\n",
        &lines);

    ASSERT(lines.len == 4);

    AsmLine *l0 = (AsmLine *)vec_get(&lines, 0);
    AsmOperand *m0 = (AsmOperand *)vec_get(&l0->operands, 1);
    ASSERT(m0->kind == ASM_OPERAND_MEM);
    ASSERT(strcmp(m0->mem_base, "rbp") == 0);
    ASSERT(m0->mem_has_disp && m0->mem_disp == -16);

    AsmLine *l1 = (AsmLine *)vec_get(&lines, 1);
    AsmOperand *m1 = (AsmOperand *)vec_get(&l1->operands, 1);
    ASSERT(strcmp(m1->mem_base, "rsp") == 0);
    ASSERT(m1->mem_has_disp && m1->mem_disp == 8);

    AsmLine *l2 = (AsmLine *)vec_get(&lines, 2);
    AsmOperand *m2 = (AsmOperand *)vec_get(&l2->operands, 1);
    ASSERT(strcmp(m2->mem_base, "rax") == 0);
    ASSERT(!m2->mem_has_disp);

    AsmLine *l3 = (AsmLine *)vec_get(&lines, 3);
    AsmOperand *m3 = (AsmOperand *)vec_get(&l3->operands, 1);
    ASSERT(m3->mem_is_rip);
    ASSERT(strcmp(m3->mem_sym, "Lstr0") == 0);

    free_lines(&lines);
}

static void test_immediates(void) {
    section("negative and 0x-hex immediate operands");
    Vec lines;
    parse_src("mov rax, -1\nmov eax, 0xff\n", &lines);

    ASSERT(lines.len == 2);
    AsmLine *l0 = (AsmLine *)vec_get(&lines, 0);
    AsmOperand *o0 = (AsmOperand *)vec_get(&l0->operands, 1);
    ASSERT(o0->kind == ASM_OPERAND_IMM && o0->imm == -1);

    AsmLine *l1 = (AsmLine *)vec_get(&lines, 1);
    AsmOperand *o1 = (AsmOperand *)vec_get(&l1->operands, 1);
    ASSERT(o1->kind == ASM_OPERAND_IMM && o1->imm == 0xff);

    free_lines(&lines);
}

static void test_zero_operand_instruction(void) {
    section("zero-operand instructions: ret, cqo");
    Vec lines;
    parse_src("ret\ncqo\n", &lines);

    ASSERT(lines.len == 2);
    AsmLine *l0 = (AsmLine *)vec_get(&lines, 0);
    ASSERT(l0->kind == ASM_LINE_INSTRUCTION);
    ASSERT(strcmp(l0->mnemonic, "ret") == 0);
    ASSERT(l0->operands.len == 0);

    AsmLine *l1 = (AsmLine *)vec_get(&lines, 1);
    ASSERT(strcmp(l1->mnemonic, "cqo") == 0);
    ASSERT(l1->operands.len == 0);

    free_lines(&lines);
}

static void test_three_operand_instruction(void) {
    section("three-operand instruction: imul rax, rax, 4");
    Vec lines;
    parse_src("imul rax, rax, 4\n", &lines);

    ASSERT(lines.len == 1);
    AsmLine *l = (AsmLine *)vec_get(&lines, 0);
    ASSERT(strcmp(l->mnemonic, "imul") == 0);
    ASSERT(l->operands.len == 3);
    ASSERT(((AsmOperand *)vec_get(&l->operands, 0))->kind == ASM_OPERAND_REG);
    ASSERT(((AsmOperand *)vec_get(&l->operands, 1))->kind == ASM_OPERAND_REG);
    AsmOperand *o2 = (AsmOperand *)vec_get(&l->operands, 2);
    ASSERT(o2->kind == ASM_OPERAND_IMM && o2->imm == 4);

    free_lines(&lines);
}

/* ---- main ---- */

int main(void) {
    printf("AsmParser unit tests\n");
    printf("=====================\n");

    test_bare_label();
    test_section_global_extern();
    test_db_comma_list();
    test_dq_symbol();
    test_label_attached_directive();
    test_mem_operands();
    test_immediates();
    test_zero_operand_instruction();
    test_three_operand_instruction();

    printf("\nResults: %d passed, %d failed, %d total\n",
           g_pass, g_fail, g_pass + g_fail);
    return g_fail > 0 ? 1 : 0;
}
