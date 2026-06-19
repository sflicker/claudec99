#ifndef CCOMPILER_PREPROCESSOR_H
#define CCOMPILER_PREPROCESSOR_H

/* Run the preprocessing pass on source text.
 * source_path is the path of the file being compiled (used to resolve
 * relative #include paths).  Pass NULL if there is no filesystem context.
 * Returns a heap-allocated string the caller must free.
 * Performs line splicing, comment stripping, #include "file" expansion,
 * object-like and function-like #define macro substitution, and
 * #ifdef/#ifndef/#else/#endif conditional compilation.
 * Exits with an error message on unsupported directives, unterminated
 * comments, missing include files, exceeded include depth, incompatible
 * macro redefinition, unmatched #else/#endif, duplicate #else, or
 * missing #endif. */
char *preprocess(const char *source, const char *source_path);

/* Like preprocess(), but pre-defines n_defines command-line macros before
 * processing the source.  Each entry in defines[] is either "NAME" (which
 * becomes #define NAME 1) or "NAME=VALUE" (which becomes #define NAME VALUE).
 * defines may be NULL when n_defines is 0. */
char *preprocess_with_defines(const char *source, const char *source_path,
                               const char **defines, int n_defines);

/* Like preprocess_with_defines(), but also accepts n_include_dirs extra
 * directories to search for #include directives.  Quoted includes search
 * the including file's directory first, then include_dirs[] in order.
 * Angle-bracket includes search include_dirs[] only, in order.
 * include_dirs may be NULL when n_include_dirs is 0.
 * When inject_preamble is non-zero, a built-in C preamble (defining
 * __builtin_va_list and related GCC builtins) is prepended before the
 * source so system headers that rely on it compile cleanly. */
char *preprocess_with_defines_and_includes(const char *source,
                                            const char *source_path,
                                            const char **defines, int n_defines,
                                            const char **include_dirs,
                                            int n_include_dirs,
                                            int inject_preamble);

#endif
