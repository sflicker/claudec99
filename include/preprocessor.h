#ifndef CCOMPILER_PREPROCESSOR_H
#define CCOMPILER_PREPROCESSOR_H

/* Run the preprocessing pass on source text.
 * Returns a heap-allocated string the caller must free.
 * Performs line splicing, comment stripping, and directive detection.
 * Exits with an error message on unsupported directives or unterminated comments. */
char *preprocess(const char *source);

#endif
