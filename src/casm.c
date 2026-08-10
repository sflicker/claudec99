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
