/*
 * asm_parser.c - Parser for the NASM-subset assembler (casm).
 *
 * Stage 177: turns the AsmToken stream from asm_lexer.c into a flat
 * Vec<AsmLine>. Each source line is either a bare label, a directive
 * (comma-separated value list for db/dd/dq, space-separated bare token
 * list otherwise), or an instruction with 0-3 operands. See the
 * Architecture note in
 * docs/stages/ClaudeC99-spec-stage-177-assembler-lexer-parser.md.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "asm_parser.h"
#include "asm_lexer.h"

/* ---- 4a: register name table ---- */

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

/* ---- 4b: directive classification ---- */

static int is_data_directive(const char *name) {
    return strcmp(name, "db") == 0 || strcmp(name, "dd") == 0 ||
           strcmp(name, "dq") == 0;
}

static int is_known_directive(const char *name) {
    static const char *names[] = {
        "section", "global", "extern", "db", "dd", "dq",
        "resb", "resd", "resq", "resw",
        "bits", "default", NULL
    };
    int i;
    for (i = 0; names[i]; i++)
        if (strcmp(names[i], name) == 0) return 1;
    return 0;
}

/* ---- 4c: parser state and error helper ---- */

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

/* ---- 4d: operand parsing ---- */

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

/* ---- 4e: line-level parsing and the driver loop ---- */

static void expect_line_end(AsmParser *p) {
    if (p->tok.type != ASM_TOK_NEWLINE && p->tok.type != ASM_TOK_EOF)
        asm_parse_error(p, "expected end of line");
    if (p->tok.type == ASM_TOK_NEWLINE)
        asm_advance(p);
}

/* Parses the argument list of a directive whose name has already been
 * consumed (`name`, e.g. "db", "resd", "global") and returns the completed
 * AsmLine. Shared by both a bare directive line ("section .text") and a
 * label-attached directive line ("Lstr0: db 1, 2, 0" / "counter: resd 1"),
 * since real codegen.c output attaches data/reservation directives
 * directly to their label on the same source line -- see the AsmLine kind
 * split below in asm_parser_parse. */
static AsmLine parse_directive_body(AsmParser *p, char *name, int line_no) {
    AsmLine line;
    line.kind = ASM_LINE_DIRECTIVE;
    line.line_no = line_no;
    line.label = NULL;
    line.directive = name;
    line.mnemonic = NULL;
    vec_init(&line.directive_args, sizeof(AsmOperand));

    if (p->tok.type != ASM_TOK_NEWLINE && p->tok.type != ASM_TOK_EOF) {
        if (is_data_directive(name)) {
            for (;;) {
                AsmOperand op;
                parse_data_value(p, &op);
                vec_push(&line.directive_args, &op);
                if (p->tok.type != ASM_TOK_COMMA) break;
                asm_advance(p);
            }
        } else {
            while (p->tok.type == ASM_TOK_IDENT || p->tok.type == ASM_TOK_INT) {
                AsmOperand op;
                parse_bare_arg(p, &op);
                vec_push(&line.directive_args, &op);
            }
        }
    }
    expect_line_end(p);
    return line;
}

void asm_parser_parse(const char *src, const char *filename, Vec *out_lines) {
    AsmParser p;
    p.filename = filename;
    asm_lexer_init(&p.lex, src);
    p.tok = asm_lexer_next(&p.lex);

    while (p.tok.type != ASM_TOK_EOF) {
        AsmLine line;
        int line_no;

        if (p.tok.type == ASM_TOK_NEWLINE) { asm_advance(&p); continue; }

        if (p.tok.type != ASM_TOK_IDENT)
            asm_parse_error(&p, "expected a label, directive, or mnemonic");

        line_no = p.tok.line;
        line.line_no = line_no;
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

                if (p.tok.type == ASM_TOK_IDENT && is_known_directive(p.tok.text)) {
                    /* "label: directive args" combined line -- emit the
                     * label and the directive as two AsmLine records
                     * sharing the same source line number. */
                    char   *dir_name;
                    AsmLine dline;
                    dir_name = asm_strdup(p.tok.text);
                    asm_advance(&p);
                    vec_push(out_lines, &line);
                    dline = parse_directive_body(&p, dir_name, line_no);
                    vec_push(out_lines, &dline);
                    continue;
                }

                expect_line_end(&p);
            } else if (is_known_directive(name)) {
                line = parse_directive_body(&p, name, line_no);
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

/* ---- asm_line_free / asm_dump_line ---- */

static void asm_operand_free(AsmOperand *op) {
    if (op->kind == ASM_OPERAND_SYM) free(op->sym);
    if (op->kind == ASM_OPERAND_MEM && op->mem_is_rip) free(op->mem_sym);
}

void asm_line_free(AsmLine *line) {
    size_t i;

    free(line->label);
    free(line->directive);
    free(line->mnemonic);

    if (line->kind == ASM_LINE_DIRECTIVE) {
        for (i = 0; i < line->directive_args.len; i++)
            asm_operand_free((AsmOperand *)vec_get(&line->directive_args, i));
        vec_free(&line->directive_args);
    } else if (line->kind == ASM_LINE_INSTRUCTION) {
        for (i = 0; i < line->operands.len; i++)
            asm_operand_free((AsmOperand *)vec_get(&line->operands, i));
        vec_free(&line->operands);
    }
}

static void asm_dump_operand(FILE *out, const AsmOperand *op) {
    switch (op->kind) {
    case ASM_OPERAND_REG:
        fprintf(out, "REG:%s", op->reg);
        break;
    case ASM_OPERAND_IMM:
        fprintf(out, "IMM:%ld", op->imm);
        break;
    case ASM_OPERAND_SYM:
        fprintf(out, "SYM:%s", op->sym);
        break;
    case ASM_OPERAND_MEM:
        if (op->size_hint) fprintf(out, "%s ", op->size_hint);
        if (op->mem_is_rip) {
            fprintf(out, "MEMREL(sym=%s)", op->mem_sym);
        } else if (op->mem_has_disp) {
            fprintf(out, "MEM(base=%s disp=%ld)", op->mem_base, op->mem_disp);
        } else {
            fprintf(out, "MEM(base=%s)", op->mem_base);
        }
        break;
    }
}

void asm_dump_line(FILE *out, const AsmLine *line) {
    size_t i;

    switch (line->kind) {
    case ASM_LINE_LABEL:
        fprintf(out, "LABEL       %s\n", line->label);
        break;

    case ASM_LINE_DIRECTIVE:
        fprintf(out, "DIRECTIVE   %s", line->directive);
        for (i = 0; i < line->directive_args.len; i++) {
            AsmOperand *op = (AsmOperand *)vec_get((Vec *)&line->directive_args, i);
            fputc(' ', out);
            if (op->kind == ASM_OPERAND_IMM)
                fprintf(out, "%ld", op->imm);
            else if (op->kind == ASM_OPERAND_SYM)
                fprintf(out, "%s", op->sym);
            else
                asm_dump_operand(out, op);
        }
        fputc('\n', out);
        break;

    case ASM_LINE_INSTRUCTION:
        fprintf(out, "INSTR       %s", line->mnemonic);
        for (i = 0; i < line->operands.len; i++) {
            AsmOperand *op = (AsmOperand *)vec_get((Vec *)&line->operands, i);
            fprintf(out, "%s", i == 0 ? "\t" : ", ");
            asm_dump_operand(out, op);
        }
        fputc('\n', out);
        break;
    }
}
