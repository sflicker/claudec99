#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "preprocessor.h"

/* Phase 1: remove backslash-newline pairs (line splicing).
 * Returns a heap-allocated string; caller must free. */
static char *splice_lines(const char *source) {
    size_t len = strlen(source);
    char *out = malloc(len + 1);
    if (!out) {
        fprintf(stderr, "error: out of memory in preprocessor\n");
        exit(1);
    }
    size_t in = 0, o = 0;
    while (source[in]) {
        if (source[in] == '\\' && source[in + 1] == '\n') {
            in += 2;
        } else {
            out[o++] = source[in++];
        }
    }
    out[o] = '\0';
    return out;
}

/* Phase 2: strip comments and detect directives.
 * Comments are replaced by a single space.
 * A '#' that is the first non-whitespace/non-comment token on a logical line
 * triggers an "unsupported preprocessor directive" error.
 * Unterminated block comments are an error.
 * Returns a heap-allocated string; caller must free. */
static char *strip_comments(const char *source) {
    size_t len = strlen(source);
    char *out = malloc(len + 1);
    if (!out) {
        fprintf(stderr, "error: out of memory in preprocessor\n");
        exit(1);
    }

    size_t in = 0, o = 0;
    /* Track whether any real (non-whitespace, non-comment) content has been
     * seen on the current logical line, to identify directive lines. */
    int line_has_content = 0;

    while (source[in]) {
        char c = source[in];

        /* Newline: reset line tracking. */
        if (c == '\n') {
            line_has_content = 0;
            out[o++] = source[in++];
            continue;
        }

        /* Whitespace: pass through, does not count as content. */
        if (isspace((unsigned char)c)) {
            out[o++] = source[in++];
            continue;
        }

        /* Line comment: skip to end of line (newline is kept). */
        if (c == '/' && source[in + 1] == '/') {
            while (source[in] && source[in] != '\n') in++;
            continue;
        }

        /* Block comment: replace with a single space. */
        if (c == '/' && source[in + 1] == '*') {
            in += 2;
            while (source[in] && !(source[in] == '*' && source[in + 1] == '/')) {
                in++;
            }
            if (!source[in]) {
                fprintf(stderr, "error: unterminated block comment\n");
                free(out);
                exit(1);
            }
            in += 2;
            out[o++] = ' ';
            continue;
        }

        /* Preprocessor directive: '#' is first non-whitespace/comment on line. */
        if (c == '#' && !line_has_content) {
            fprintf(stderr, "error: unsupported preprocessor directive\n");
            free(out);
            exit(1);
        }

        /* String literal: pass through unchanged so the lexer handles it.
         * This prevents '//' or '/*' inside strings from being treated as comments. */
        if (c == '"') {
            line_has_content = 1;
            out[o++] = source[in++];
            while (source[in] && source[in] != '"' && source[in] != '\n') {
                if (source[in] == '\\' && source[in + 1]) {
                    out[o++] = source[in++];
                }
                out[o++] = source[in++];
            }
            if (source[in] == '"') out[o++] = source[in++];
            continue;
        }

        /* Character literal: pass through unchanged. */
        if (c == '\'') {
            line_has_content = 1;
            out[o++] = source[in++];
            while (source[in] && source[in] != '\'' && source[in] != '\n') {
                if (source[in] == '\\' && source[in + 1]) {
                    out[o++] = source[in++];
                }
                out[o++] = source[in++];
            }
            if (source[in] == '\'') out[o++] = source[in++];
            continue;
        }

        /* Regular character. */
        line_has_content = 1;
        out[o++] = source[in++];
    }
    out[o] = '\0';
    return out;
}

char *preprocess(const char *source) {
    char *spliced = splice_lines(source);
    char *result = strip_comments(spliced);
    free(spliced);
    return result;
}
