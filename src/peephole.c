/*
 * peephole.c - Post-codegen peephole optimizer.
 *
 * Stage 155: infrastructure -- sliding window engine over the emitted
 *            NASM text; patterns expressed as matcher + replacer pairs.
 *            No patterns registered at this stage; the pass is a no-op.
 *
 * Activated at -O2 (implies -O1: AST optimizer also runs).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "peephole.h"
#include "util.h"

/*
 * read_lines -- read all lines from path into a heap-allocated array.
 *
 * Lines are stored without trailing newlines.  Sets *out_lines and
 * *out_count.  Returns 0 on success, 1 on I/O or allocation failure.
 */
static int read_lines(const char *path, char ***out_lines, int *out_count) {
    FILE  *f;
    char   buf[4096];
    int    cap, n, i;
    char **lines;
    char **grown;
    size_t len;

    f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "peephole: cannot open '%s' for reading\n", path);
        return 1;
    }

    cap   = 64;
    n     = 0;
    lines = (char **)malloc((size_t)cap * sizeof(char *));
    if (!lines) {
        fprintf(stderr, "peephole: out of memory\n");
        fclose(f);
        return 1;
    }

    while (fgets(buf, (int)sizeof(buf), f)) {
        len = strlen(buf);
        /* Strip trailing newline characters. */
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
            len--;
        buf[len] = '\0';

        if (n == cap) {
            cap *= 2;
            grown = (char **)realloc(lines, (size_t)cap * sizeof(char *));
            if (!grown) {
                fprintf(stderr, "peephole: out of memory\n");
                for (i = 0; i < n; i++) free(lines[i]);
                free(lines);
                fclose(f);
                return 1;
            }
            lines = grown;
        }
        lines[n] = util_strdup(buf);
        n++;
    }

    fclose(f);
    *out_lines = lines;
    *out_count = n;
    return 0;
}

/*
 * write_lines -- write n lines to path, each followed by a newline.
 *
 * Returns 0 on success, 1 on I/O failure.
 */
static int write_lines(const char *path, char * const *lines, int n) {
    FILE *f;
    int   i;

    f = fopen(path, "w");
    if (!f) {
        fprintf(stderr, "peephole: cannot open '%s' for writing\n", path);
        return 1;
    }

    for (i = 0; i < n; i++) {
        if (fprintf(f, "%s\n", lines[i]) < 0) {
            fprintf(stderr, "peephole: write error on '%s'\n", path);
            fclose(f);
            return 1;
        }
    }

    fclose(f);
    return 0;
}

/*
 * peephole_apply -- run all patterns over the line array in place.
 *
 * A single forward pass slides a window of pattern->window_size lines.
 * The first matching pattern fires; scanning resumes after the replacement.
 */
void peephole_apply(char ***lines, int *nlines,
                    const PeepholePattern *patterns, int n_pats) {
    int    i, p, matched, w, j, k, new_n, delta, out_count;
    char  *out_buf[PEEPHOLE_WINDOW_MAX];
    char **old_arr;
    char **new_arr;

    if (n_pats == 0 || patterns == NULL) return;

    i = 0;
    while (i < *nlines) {
        matched = 0;
        for (p = 0; p < n_pats && !matched; p++) {
            w = patterns[p].window_size;
            if (i + w > *nlines) continue;

            if (!patterns[p].matcher(
                    (const char * const *)(*lines + i), w))
                continue;

            out_count = 0;
            patterns[p].replacer(
                (const char * const *)(*lines + i), w,
                out_buf, &out_count);

            /* Build a new array: prefix + replacements + suffix. */
            old_arr = *lines;
            delta   = out_count - w;
            new_n   = *nlines + delta;
            new_arr = (char **)malloc(
                (size_t)(new_n > 0 ? new_n : 1) * sizeof(char *));
            if (!new_arr) {
                fprintf(stderr, "peephole: out of memory\n");
                for (k = 0; k < out_count; k++) free(out_buf[k]);
                return;
            }

            /* Copy prefix [0 .. i-1]. */
            for (j = 0; j < i; j++) new_arr[j] = old_arr[j];
            /* Insert replacement lines. */
            for (k = 0; k < out_count; k++) new_arr[i + k] = out_buf[k];
            /* Copy suffix [i+w .. *nlines-1] shifted by delta. */
            for (j = i + w; j < *nlines; j++)
                new_arr[j + delta] = old_arr[j];

            /* Free the replaced window lines, then the old array. */
            for (k = 0; k < w; k++) free(old_arr[i + k]);
            free(old_arr);

            *lines  = new_arr;
            *nlines = new_n;

            matched = 1;
            i += out_count; /* resume after replacement output */
        }
        if (!matched) i++;
    }
}

/*
 * peephole_run_file -- read path, apply all patterns, write result back.
 */
int peephole_run_file(const char *path,
                      const PeepholePattern *patterns, int n_pats) {
    char **lines;
    int    n, k;

    if (read_lines(path, &lines, &n) != 0) return 1;

    peephole_apply(&lines, &n, patterns, n_pats);

    if (write_lines(path, (char * const *)lines, n) != 0) {
        for (k = 0; k < n; k++) free(lines[k]);
        free(lines);
        return 1;
    }

    for (k = 0; k < n; k++) free(lines[k]);
    free(lines);
    return 0;
}
