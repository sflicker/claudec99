#ifndef CCOMPILER_PREPROCESSOR_H
#define CCOMPILER_PREPROCESSOR_H

/* Run the preprocessing pass on source text.
 * source_path is the path of the file being compiled (used to resolve
 * relative #include paths).  Pass NULL if there is no filesystem context.
 * Returns a heap-allocated string the caller must free.
 * Performs line splicing, comment stripping, #include "file" expansion,
 * and directive detection.
 * Exits with an error message on unsupported directives, unterminated
 * comments, missing include files, or exceeded include depth. */
char *preprocess(const char *source, const char *source_path);

#endif
