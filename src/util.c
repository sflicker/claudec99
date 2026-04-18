#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

char *read_file(const char *path) {
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

void make_output_path(char *out, size_t out_size, const char *input_path) {
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
