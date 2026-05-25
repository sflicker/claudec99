#ifndef CCOMPILER_UTIL_H
#define CCOMPILER_UTIL_H

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

char *read_file(const char *path);
void make_output_path(char *out, size_t out_size, const char *input_path);

/* Error counting and recovery for --max-errors support.
 * g_max_errors: 0 = unlimited; N > 0 = exit after N errors.
 * g_error_jmp_valid: set while parse_translation_unit's setjmp is active. */
extern int    g_max_errors;
extern int    g_error_count;
extern jmp_buf g_error_jmp;
extern int    g_error_jmp_valid;

__attribute__((noreturn, format(printf, 1, 2)))
void compile_error(const char *fmt, ...);

#endif
