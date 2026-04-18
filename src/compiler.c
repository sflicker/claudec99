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
#include "util.h"

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
