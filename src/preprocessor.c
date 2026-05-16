#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "preprocessor.h"

#define MAX_INCLUDE_DEPTH 64

/* ---- Growable output buffer ------------------------------------------ */

typedef struct {
    char  *data;
    size_t len;
    size_t cap;
} GrowBuf;

static void gbuf_init(GrowBuf *g, size_t cap) {
    if (cap < 16) cap = 16;
    g->data = malloc(cap);
    if (!g->data) { fprintf(stderr, "error: out of memory\n"); exit(1); }
    g->len = 0;
    g->cap = cap;
}

static void gbuf_push(GrowBuf *g, char c) {
    if (g->len + 1 >= g->cap) {
        g->cap *= 2;
        g->data = realloc(g->data, g->cap);
        if (!g->data) { fprintf(stderr, "error: out of memory\n"); exit(1); }
    }
    g->data[g->len++] = c;
}

static void gbuf_append(GrowBuf *g, const char *s, size_t n) {
    while (g->len + n + 1 > g->cap) {
        g->cap = g->cap * 2 + n;
        g->data = realloc(g->data, g->cap);
        if (!g->data) { fprintf(stderr, "error: out of memory\n"); exit(1); }
    }
    memcpy(g->data + g->len, s, n);
    g->len += n;
}

/* ---- Helpers ---------------------------------------------------------- */

/* Return heap-allocated directory portion of path (caller frees). */
static char *get_dir(const char *path) {
    if (!path) return strdup(".");
    const char *slash = strrchr(path, '/');
    if (!slash) return strdup(".");
    size_t len = (size_t)(slash - path);
    char *dir = malloc(len + 1);
    if (!dir) { fprintf(stderr, "error: out of memory\n"); exit(1); }
    memcpy(dir, path, len);
    dir[len] = '\0';
    return dir;
}

/* ---- Phase 1: line splicing ------------------------------------------ */

static char *splice_lines(const char *source) {
    size_t len = strlen(source);
    char *out = malloc(len + 1);
    if (!out) { fprintf(stderr, "error: out of memory\n"); exit(1); }
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

/* ---- Forward declaration for mutual recursion ------------------------- */

static char *preprocess_internal(const char *source, const char *source_path, int depth);

/* ---- Read and recursively preprocess an included file ---------------- */

static char *preprocess_file(const char *path, int depth) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "error: cannot open include file '%s'\n", path);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long flen = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(flen + 1);
    if (!buf) { fprintf(stderr, "error: out of memory\n"); fclose(f); exit(1); }
    fread(buf, 1, flen, f);
    buf[flen] = '\0';
    fclose(f);
    char *result = preprocess_internal(buf, path, depth);
    free(buf);
    return result;
}

/* ---- Phase 2: strip comments and expand #include directives ---------- */

static char *preprocess_internal(const char *source, const char *source_path, int depth) {
    if (depth > MAX_INCLUDE_DEPTH) {
        fprintf(stderr, "error: maximum include depth exceeded (possible recursive include)\n");
        exit(1);
    }

    char *spliced = splice_lines(source);
    const char *s = spliced;
    size_t slen = strlen(s);

    GrowBuf out;
    gbuf_init(&out, slen + 1);

    size_t in = 0;
    int line_has_content = 0;

    while (s[in]) {
        char c = s[in];

        /* Newline: reset line tracking. */
        if (c == '\n') {
            line_has_content = 0;
            gbuf_push(&out, s[in++]);
            continue;
        }

        /* Whitespace: pass through, does not count as content. */
        if (isspace((unsigned char)c)) {
            gbuf_push(&out, s[in++]);
            continue;
        }

        /* Line comment: skip to end of line (newline is kept). */
        if (c == '/' && s[in + 1] == '/') {
            while (s[in] && s[in] != '\n') in++;
            continue;
        }

        /* Block comment: replace with a single space. */
        if (c == '/' && s[in + 1] == '*') {
            in += 2;
            while (s[in] && !(s[in] == '*' && s[in + 1] == '/')) {
                in++;
            }
            if (!s[in]) {
                fprintf(stderr, "error: unterminated block comment\n");
                free(out.data);
                free(spliced);
                exit(1);
            }
            in += 2;
            gbuf_push(&out, ' ');
            continue;
        }

        /* Preprocessor directive: '#' is first non-whitespace on line. */
        if (c == '#' && !line_has_content) {
            in++; /* skip '#' */
            while (s[in] == ' ' || s[in] == '\t') in++;

            /* #include "filename" */
            if (strncmp(s + in, "include", 7) == 0 &&
                !isalnum((unsigned char)s[in + 7]) && s[in + 7] != '_') {
                in += 7;
                while (s[in] == ' ' || s[in] == '\t') in++;

                if (s[in] != '"') {
                    fprintf(stderr, "error: expected '\"' after #include\n");
                    free(out.data);
                    free(spliced);
                    exit(1);
                }
                in++; /* skip opening '"' */

                size_t fname_start = in;
                while (s[in] && s[in] != '"' && s[in] != '\n') in++;
                if (s[in] != '"') {
                    fprintf(stderr, "error: unterminated filename in #include\n");
                    free(out.data);
                    free(spliced);
                    exit(1);
                }
                size_t fname_len = in - fname_start;
                in++; /* skip closing '"' */

                /* Discard rest of directive line. */
                while (s[in] && s[in] != '\n') in++;

                /* Build the include path relative to the current file's dir. */
                char *dir = get_dir(source_path);
                size_t dir_len = strlen(dir);
                char *include_path = malloc(dir_len + 1 + fname_len + 1);
                if (!include_path) { fprintf(stderr, "error: out of memory\n"); exit(1); }
                memcpy(include_path, dir, dir_len);
                include_path[dir_len] = '/';
                memcpy(include_path + dir_len + 1, s + fname_start, fname_len);
                include_path[dir_len + 1 + fname_len] = '\0';
                free(dir);

                char *included = preprocess_file(include_path, depth + 1);
                free(include_path);
                gbuf_append(&out, included, strlen(included));
                free(included);
                continue;
            }

            /* All other directives are unsupported. */
            fprintf(stderr, "error: unsupported preprocessor directive\n");
            free(out.data);
            free(spliced);
            exit(1);
        }

        /* String literal: pass through unchanged. */
        if (c == '"') {
            line_has_content = 1;
            gbuf_push(&out, s[in++]);
            while (s[in] && s[in] != '"' && s[in] != '\n') {
                if (s[in] == '\\' && s[in + 1]) {
                    gbuf_push(&out, s[in++]);
                }
                gbuf_push(&out, s[in++]);
            }
            if (s[in] == '"') gbuf_push(&out, s[in++]);
            continue;
        }

        /* Character literal: pass through unchanged. */
        if (c == '\'') {
            line_has_content = 1;
            gbuf_push(&out, s[in++]);
            while (s[in] && s[in] != '\'' && s[in] != '\n') {
                if (s[in] == '\\' && s[in + 1]) {
                    gbuf_push(&out, s[in++]);
                }
                gbuf_push(&out, s[in++]);
            }
            if (s[in] == '\'') gbuf_push(&out, s[in++]);
            continue;
        }

        /* Regular character. */
        line_has_content = 1;
        gbuf_push(&out, s[in++]);
    }

    gbuf_push(&out, '\0');
    free(spliced);
    return out.data;
}

char *preprocess(const char *source, const char *source_path) {
    return preprocess_internal(source, source_path ? source_path : ".", 0);
}
