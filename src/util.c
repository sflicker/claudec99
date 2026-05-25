#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

/* Error counting and recovery globals. */
int     g_max_errors    = 1;
int     g_error_count   = 0;
jmp_buf g_error_jmp;
int     g_error_jmp_valid = 0;

/* Report a compilation error.
 * Always prints the message. Increments g_error_count. Exits when the
 * error limit is reached (g_max_errors > 0 && g_error_count >= g_max_errors).
 * Otherwise performs a long jump to the recovery point in
 * parse_translation_unit if one is active; exits unconditionally if not. */
__attribute__((noreturn))
void compile_error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    g_error_count++;
    if (g_max_errors > 0 && g_error_count >= g_max_errors) {
        exit(1);
    }
    if (g_error_jmp_valid) {
        longjmp(g_error_jmp, 1);
    }
    exit(1);
}

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
