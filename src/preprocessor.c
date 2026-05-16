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

/* ---- Macro table ----------------------------------------------------- */

typedef struct {
    char *name;
    char *replacement;
} MacroDef;

typedef struct {
    MacroDef *defs;
    size_t    count;
    size_t    cap;
} MacroTable;

static void macro_table_init(MacroTable *t) {
    t->defs  = NULL;
    t->count = 0;
    t->cap   = 0;
}

static void macro_table_free(MacroTable *t) {
    for (size_t i = 0; i < t->count; i++) {
        free(t->defs[i].name);
        free(t->defs[i].replacement);
    }
    free(t->defs);
}

static MacroDef *macro_find(MacroTable *t, const char *name, size_t len) {
    for (size_t i = 0; i < t->count; i++) {
        if (strlen(t->defs[i].name) == len &&
            strncmp(t->defs[i].name, name, len) == 0)
            return &t->defs[i];
    }
    return NULL;
}

static void macro_define(MacroTable *t, const char *name, size_t nlen,
                          const char *repl, size_t rlen) {
    MacroDef *existing = macro_find(t, name, nlen);
    if (existing) {
        if (strlen(existing->replacement) == rlen &&
            strncmp(existing->replacement, repl, rlen) == 0)
            return;
        char tmp[256];
        size_t copy = nlen < sizeof(tmp) - 1 ? nlen : sizeof(tmp) - 1;
        memcpy(tmp, name, copy);
        tmp[copy] = '\0';
        fprintf(stderr, "error: macro '%s' redefined with incompatible replacement\n", tmp);
        exit(1);
    }

    if (t->count == t->cap) {
        t->cap = t->cap * 2 + 8;
        t->defs = realloc(t->defs, t->cap * sizeof(MacroDef));
        if (!t->defs) { fprintf(stderr, "error: out of memory\n"); exit(1); }
    }

    char *nname = malloc(nlen + 1);
    char *nrepl = malloc(rlen + 1);
    if (!nname || !nrepl) { fprintf(stderr, "error: out of memory\n"); exit(1); }
    memcpy(nname, name, nlen);
    nname[nlen] = '\0';
    memcpy(nrepl, repl, rlen);
    nrepl[rlen] = '\0';

    t->defs[t->count].name        = nname;
    t->defs[t->count].replacement = nrepl;
    t->count++;
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

static char *preprocess_internal(const char *source, const char *source_path,
                                  int depth, MacroTable *macros);

/* ---- Read and recursively preprocess an included file ---------------- */

static char *preprocess_file(const char *path, int depth, MacroTable *macros) {
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
    char *result = preprocess_internal(buf, path, depth, macros);
    free(buf);
    return result;
}

/* ---- Phase 2: strip comments, expand directives and macros ----------- */

static char *preprocess_internal(const char *source, const char *source_path,
                                  int depth, MacroTable *macros) {
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

                char *included = preprocess_file(include_path, depth + 1, macros);
                free(include_path);
                gbuf_append(&out, included, strlen(included));
                free(included);
                continue;
            }

            /* #define NAME [replacement] */
            if (strncmp(s + in, "define", 6) == 0 &&
                !isalnum((unsigned char)s[in + 6]) && s[in + 6] != '_') {
                in += 6;
                while (s[in] == ' ' || s[in] == '\t') in++;

                if (!isalpha((unsigned char)s[in]) && s[in] != '_') {
                    fprintf(stderr, "error: expected macro name after #define\n");
                    free(out.data);
                    free(spliced);
                    exit(1);
                }
                size_t name_start = in;
                while (s[in] && (isalnum((unsigned char)s[in]) || s[in] == '_'))
                    in++;
                size_t name_len = in - name_start;

                /* Reject function-like macros. */
                if (s[in] == '(') {
                    fprintf(stderr, "error: function-like macros are not supported\n");
                    free(out.data);
                    free(spliced);
                    exit(1);
                }

                /* Skip whitespace separating name from replacement. */
                while (s[in] == ' ' || s[in] == '\t') in++;

                /* Replacement: rest of line, trailing whitespace trimmed. */
                size_t repl_start = in;
                while (s[in] && s[in] != '\n') in++;
                size_t repl_end = in;
                while (repl_end > repl_start &&
                       (s[repl_end - 1] == ' ' || s[repl_end - 1] == '\t'))
                    repl_end--;

                macro_define(macros,
                             s + name_start, name_len,
                             s + repl_start, repl_end - repl_start);
                /* Directive line consumed; newline handled on next iteration. */
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

        /* Identifier: expand if it names a macro, otherwise pass through. */
        if (isalpha((unsigned char)c) || c == '_') {
            size_t id_start = in;
            while (s[in] && (isalnum((unsigned char)s[in]) || s[in] == '_'))
                in++;
            size_t id_len = in - id_start;
            MacroDef *def = macro_find(macros, s + id_start, id_len);
            if (def) {
                gbuf_append(&out, def->replacement, strlen(def->replacement));
            } else {
                gbuf_append(&out, s + id_start, id_len);
            }
            line_has_content = 1;
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
    MacroTable macros;
    macro_table_init(&macros);
    char *result = preprocess_internal(source,
                                       source_path ? source_path : ".",
                                       0, &macros);
    macro_table_free(&macros);
    return result;
}
