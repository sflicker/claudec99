/*
 * peephole.c - Post-codegen peephole optimizer.
 *
 * Stage 155: infrastructure -- sliding window engine over the emitted
 *            NASM text; patterns expressed as matcher + replacer pairs.
 *            No patterns registered at this stage; the pass is a no-op.
 *
 * Stage 157: zero-register idiom -- `mov REG, 0` -> `xor REG32, REG32`
 *            for all 64-bit and 32-bit GP registers; single-line window.
 *
 * Stage 164: no-op move elimination -- `mov REG, REG` (same src/dst) -> remove.
 *
 * Stage 165: push/pop pair collapse -- `push rX` / `pop rY` -> `mov rY, rX`.
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

/* -----------------------------------------------------------------------
 * Zero-register idiom: mov REG, 0  ->  xor REG32, REG32
 *
 * Parallel tables: g_zr_src[i] is the source register name in the mov
 * instruction; g_zr_dst[i] is the 32-bit register name for the xor.
 * Covers all 64-bit GP registers (writing a 32-bit reg zeroes the upper
 * 32 bits on x86-64) and their 32-bit forms (also valid to xor).
 * ----------------------------------------------------------------------- */
static const char * const g_zr_src[] = {
    "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
    "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
    "eax", "ebx", "ecx", "edx", "esi", "edi",
    NULL
};
static const char * const g_zr_dst[] = {
    "eax", "ebx", "ecx", "edx", "esi", "edi",
    "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d",
    "eax", "ebx", "ecx", "edx", "esi", "edi",
    NULL
};

/*
 * zr_find_reg -- scan line for "mov REG, 0" (after leading whitespace).
 * Returns the table index on match, -1 otherwise.
 */
static int zr_find_reg(const char *line) {
    const char *p;
    int i;
    size_t rlen;

    /* Skip leading whitespace. */
    p = line;
    while (*p == ' ' || *p == '\t') p++;

    /* Must start with "mov ". */
    if (strncmp(p, "mov ", 4) != 0) return -1;
    p += 4;

    /* Try each known source register. */
    for (i = 0; g_zr_src[i] != NULL; i++) {
        rlen = strlen(g_zr_src[i]);
        if (strncmp(p, g_zr_src[i], rlen) == 0) {
            /* Must be followed by exactly ", 0" and nothing else. */
            if (strcmp(p + rlen, ", 0") == 0) return i;
        }
    }
    return -1;
}

static int match_zero_reg(const char **win, int n) {
    (void)n;
    return zr_find_reg(win[0]) >= 0;
}

static void replace_zero_reg(const char **win, int n,
                              char **out, int *out_count) {
    const char *p;
    char buf[64];
    int  idx;
    size_t prefix_len;

    (void)n;

    idx = zr_find_reg(win[0]);

    /* Measure leading whitespace to preserve indentation. */
    p = win[0];
    while (*p == ' ' || *p == '\t') p++;
    prefix_len = (size_t)(p - win[0]);

    if (prefix_len >= sizeof(buf) - 1) prefix_len = sizeof(buf) - 2;
    strncpy(buf, win[0], prefix_len);
    buf[prefix_len] = '\0';
    strncat(buf, "xor ", sizeof(buf) - strlen(buf) - 1);
    strncat(buf, g_zr_dst[idx], sizeof(buf) - strlen(buf) - 1);
    strncat(buf, ", ", sizeof(buf) - strlen(buf) - 1);
    strncat(buf, g_zr_dst[idx], sizeof(buf) - strlen(buf) - 1);

    out[0] = util_strdup(buf);
    *out_count = 1;
}

/* -----------------------------------------------------------------------
 * No-op move elimination: mov REG, REG (same src and dst) -> remove.
 *
 * Matches any register width: 64-bit (rax), 32-bit (eax), 16-bit (ax),
 * 8-bit (al), and extended registers (r8-r15 in all widths).
 * The entire line is deleted (out_count = 0).
 * ----------------------------------------------------------------------- */

static int match_nop_move(const char **win, int n) {
    const char *p;
    const char *dst_start;
    size_t dst_len;

    (void)n;
    p = win[0];
    while (*p == ' ' || *p == '\t') p++;
    if (strncmp(p, "mov ", 4) != 0) return 0;
    p += 4;

    dst_start = p;
    while (*p != '\0' && *p != ',') p++;
    if (*p != ',') return 0;
    dst_len = (size_t)(p - dst_start);
    p++; /* skip ',' */
    if (*p != ' ') return 0;
    p++; /* skip ' ' */

    return (strncmp(p, dst_start, dst_len) == 0 && p[dst_len] == '\0');
}

static void replace_nop_move(const char **win, int n,
                              char **out, int *out_count) {
    (void)win;
    (void)n;
    (void)out;
    *out_count = 0;
}

/* -----------------------------------------------------------------------
 * Push/pop pair collapse: push rX / pop rY (adjacent) -> mov rY, rX.
 *
 * If both registers are the same (push rX / pop rX), the pair is a
 * no-op and both lines are deleted (out_count = 0).
 * ----------------------------------------------------------------------- */

/*
 * pp_extract_reg -- extract the register name from a "push REG" or "pop REG"
 * line.  mnemonic is the expected prefix ("push " or "pop ").
 * Returns 1 on success, 0 on parse failure.
 */
static int pp_extract_reg(const char *line, const char *mnemonic,
                           char *dst, size_t dst_size) {
    const char *p;
    const char *reg;
    size_t mlen;
    size_t rlen;

    p    = line;
    mlen = strlen(mnemonic);
    while (*p == ' ' || *p == '\t') p++;
    if (strncmp(p, mnemonic, mlen) != 0) return 0;
    p += mlen;

    reg  = p;
    rlen = 0;
    while (*p != '\0') { p++; rlen++; }
    if (rlen == 0 || rlen >= dst_size) return 0;

    strncpy(dst, reg, rlen);
    dst[rlen] = '\0';
    return 1;
}

static int match_push_pop(const char **win, int n) {
    char r0[32];
    char r1[32];
    (void)n;
    if (!pp_extract_reg(win[0], "push ", r0, sizeof(r0))) return 0;
    if (!pp_extract_reg(win[1], "pop ",  r1, sizeof(r1))) return 0;
    return 1;
}

static void replace_push_pop(const char **win, int n,
                              char **out, int *out_count) {
    char        r0[32];
    char        r1[32];
    char        buf[80];
    const char *p;
    size_t      prefix_len;

    (void)n;

    pp_extract_reg(win[0], "push ", r0, sizeof(r0));
    pp_extract_reg(win[1], "pop ",  r1, sizeof(r1));

    /* Measure leading whitespace from the push line. */
    p = win[0];
    while (*p == ' ' || *p == '\t') p++;
    prefix_len = (size_t)(p - win[0]);
    if (prefix_len >= sizeof(buf) - 1) prefix_len = sizeof(buf) - 2;

    /* Same register: the pair is a no-op; delete both lines. */
    if (strcmp(r0, r1) == 0) {
        *out_count = 0;
        return;
    }

    strncpy(buf, win[0], prefix_len);
    buf[prefix_len] = '\0';
    strncat(buf, "mov ", sizeof(buf) - strlen(buf) - 1);
    strncat(buf, r1,    sizeof(buf) - strlen(buf) - 1);
    strncat(buf, ", ",  sizeof(buf) - strlen(buf) - 1);
    strncat(buf, r0,    sizeof(buf) - strlen(buf) - 1);

    out[0]     = util_strdup(buf);
    *out_count = 1;
}

/* Built-in pattern table -- initialized at first call because C0 does not
   support function-pointer initializers in struct aggregate literals. */
static PeepholePattern g_builtin_patterns[3];
static int             g_builtin_patterns_ready = 0;

const PeepholePattern *peephole_builtin_patterns(int *n_pats) {
    if (!g_builtin_patterns_ready) {
        g_builtin_patterns[0].window_size = 1;
        g_builtin_patterns[0].matcher     = match_zero_reg;
        g_builtin_patterns[0].replacer    = replace_zero_reg;
        g_builtin_patterns[1].window_size = 1;
        g_builtin_patterns[1].matcher     = match_nop_move;
        g_builtin_patterns[1].replacer    = replace_nop_move;
        g_builtin_patterns[2].window_size = 2;
        g_builtin_patterns[2].matcher     = match_push_pop;
        g_builtin_patterns[2].replacer    = replace_push_pop;
        g_builtin_patterns_ready          = 1;
    }
    *n_pats = 3;
    return g_builtin_patterns;
}

/* -----------------------------------------------------------------------
 * Sliding window engine
 * ----------------------------------------------------------------------- */

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
    char **arr;

    if (n_pats == 0 || patterns == NULL) return;

    i = 0;
    while (i < *nlines) {
        arr     = *lines;
        matched = 0;
        for (p = 0; p < n_pats && !matched; p++) {
            w = patterns[p].window_size;
            if (i + w > *nlines) continue;

            if (!patterns[p].matcher(
                    (const char **)(arr + i), w))
                continue;

            out_count = 0;
            patterns[p].replacer(
                (const char **)(arr + i), w,
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
