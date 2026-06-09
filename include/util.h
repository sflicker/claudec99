#ifndef CCOMPILER_UTIL_H
#define CCOMPILER_UTIL_H

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

/* Stage 83: portable wrappers for GNU function attributes. On GNU/clang
 * these expand to the useful diagnostics; everywhere else they expand to
 * nothing so the project compiles clean under strict ISO C99. */
#if defined(__GNUC__)
#  define CC_NORETURN      __attribute__((noreturn))
#  define CC_PRINTF(a, b)  __attribute__((format(printf, a, b)))
#else
#  define CC_NORETURN
#  define CC_PRINTF(a, b)
#endif

char *read_file(const char *path);
void make_output_path(char *out, size_t out_size, const char *input_path);

/* Stage 83: ISO C99 replacement for POSIX strdup (not in C99). Returns a
 * malloc'd copy of s, or NULL if s is NULL or allocation fails. */
char *util_strdup(const char *s);

/* Error counting and recovery for --max-errors support.
 * g_max_errors: 0 = unlimited; N > 0 = exit after N errors.
 * g_error_jmp_valid: set while parse_translation_unit's setjmp is active. */
extern int    g_max_errors;
extern int    g_error_count;
extern jmp_buf g_error_jmp;
extern int    g_error_jmp_valid;

/* Stage 70-03: global flag set by -Werror; used by compile_warning_at. */
extern int g_warnings_are_errors;

/* Stage 86: source position for the construct currently being compiled,
 * stamped onto AST nodes at parse time and tracked by codegen as it
 * descends. Plain compile_error() prefixes its message with
 * file:line:col when g_error_src_file is non-NULL and g_error_src_line > 0,
 * giving codegen/semantic errors the same position info parser errors
 * already carry. Left NULL/0 during parsing, where PARSER_ERROR supplies
 * its own position. */
extern const char *g_error_src_file;
extern int         g_error_src_line;
extern int         g_error_src_col;

/* Stage 96: reset per-file error state between files in multi-file
 * compilation. Zeroes g_error_count, g_error_src_*, g_error_jmp_valid.
 * Does NOT reset g_max_errors or g_warnings_are_errors. */
void reset_error_state(void);

CC_NORETURN CC_PRINTF(1, 2)
void compile_error(const char *fmt, ...);

/* Stage 70-03: emit an error prefixed with file:line:col when file is non-NULL. */
CC_NORETURN CC_PRINTF(4, 5)
void compile_error_at(const char *file, int line, int col, const char *fmt, ...);

/* Stage 70-03: emit a warning prefixed with file:line:col when file is non-NULL.
 * Becomes a fatal error when g_warnings_are_errors is set. */
CC_PRINTF(4, 5)
void compile_warning_at(const char *file, int line, int col, const char *fmt, ...);

#endif
