#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

/* Error counting and recovery globals. */
int     g_max_errors    = 1;
int     g_error_count   = 0;
jmp_buf g_error_jmp;
int     g_error_jmp_valid = 0;
int     g_warnings_are_errors = 0;

/* Stage 86: position of the construct currently being compiled (see util.h). */
const char *g_error_src_file = NULL;
int         g_error_src_line = 0;
int         g_error_src_col  = 0;

/* Core error-reporting logic shared by compile_error and compile_error_at. */
CC_NORETURN
static void do_compile_error(const char *fmt, va_list ap) {
    vfprintf(stderr, fmt, ap);
    g_error_count++;
    if (g_max_errors > 0 && g_error_count >= g_max_errors) {
        exit(1);
    }
    if (g_error_jmp_valid) {
        longjmp(g_error_jmp, 1);
    }
    exit(1);
}

/* Report a compilation error. When codegen has stamped a position via the
 * g_error_src_* globals (set from the AST node currently being compiled),
 * the message is prefixed with file:line:col so semantic errors carry the
 * same position info parser errors do. */
CC_NORETURN
void compile_error(const char *fmt, ...) {
    if (g_error_src_file && g_error_src_line > 0)
        fprintf(stderr, "%s:%d:%d: ", g_error_src_file,
                g_error_src_line, g_error_src_col);
    va_list ap;
    va_start(ap, fmt);
    do_compile_error(fmt, ap);
}

/* Stage 70-03: report a compilation error with a file:line:col: prefix.
 * When file is NULL or line is 0, behaves like compile_error. */
CC_NORETURN
void compile_error_at(const char *file, int line, int col, const char *fmt, ...) {
    if (file && line > 0)
        fprintf(stderr, "%s:%d:%d: ", file, line, col);
    va_list ap;
    va_start(ap, fmt);
    do_compile_error(fmt, ap);
}

/* Stage 70-03: report a compilation warning with a file:line:col: prefix.
 * Becomes a fatal error when g_warnings_are_errors is set. */
void compile_warning_at(const char *file, int line, int col, const char *fmt, ...) {
    if (file && line > 0)
        fprintf(stderr, "%s:%d:%d: ", file, line, col);
    va_list ap;
    va_start(ap, fmt);
    if (g_warnings_are_errors) {
        do_compile_error(fmt, ap);
    } else {
        vfprintf(stderr, fmt, ap);
        va_end(ap);
    }
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

/* Stage 83: ISO C99 replacement for POSIX strdup. */
char *util_strdup(const char *s) {
    if (!s) {
        return NULL;
    }
    size_t len = strlen(s) + 1;
    char *copy = malloc(len);
    if (copy) {
        memcpy(copy, s, len);
    }
    return copy;
}
