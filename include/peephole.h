#ifndef CCOMPILER_PEEPHOLE_H
#define CCOMPILER_PEEPHOLE_H

#include <stdio.h>

/* Maximum number of consecutive lines in one peephole window. */
#define PEEPHOLE_WINDOW_MAX 4

/*
 * PeepholeMatcher -- return 1 if the window of `n` lines matches.
 * `win` is a read-only array of `n` trimmed strings (no trailing newline).
 */
typedef int (*PeepholeMatcher)(const char **win, int n);

/*
 * PeepholeReplacer -- write replacement lines for a matched window.
 * `out` is a caller-provided array of PEEPHOLE_WINDOW_MAX pointers.
 * The replacer fills out[0..*out_count-1] with heap-allocated strings
 * (no trailing newline) and sets *out_count (0 = delete all n lines).
 */
typedef void (*PeepholeReplacer)(const char **win, int n,
                                  char **out, int *out_count);

typedef struct {
    int              window_size; /* 1 to PEEPHOLE_WINDOW_MAX */
    PeepholeMatcher  matcher;
    PeepholeReplacer replacer;
} PeepholePattern;

/*
 * peephole_apply -- run patterns over a mutable line array.
 *
 * *lines   : pointer to a heap-allocated array of heap-allocated strings
 *            (no trailing newlines).  On return, *lines and *nlines
 *            reflect the post-optimization state.  Replaced lines are
 *            freed; replacement lines are caller-owned heap strings.
 * *nlines  : line count; updated in place.
 * patterns : pattern table (may be NULL when n_pats == 0).
 * n_pats   : number of entries in patterns.
 *
 * Patterns are tried in registration order; the first match fires.
 * Scanning resumes after the replacement window (no re-scan).
 */
void peephole_apply(char ***lines, int *nlines,
                    const PeepholePattern *patterns, int n_pats);

/*
 * peephole_run_file -- read path, apply patterns, write result back.
 *
 * Returns 0 on success, 1 on I/O failure (error printed to stderr).
 */
int peephole_run_file(const char *path,
                      const PeepholePattern *patterns, int n_pats);

#endif /* CCOMPILER_PEEPHOLE_H */
