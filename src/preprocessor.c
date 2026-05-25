#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "preprocessor.h"

#define MAX_INCLUDE_DEPTH 64
#define MAX_COND_DEPTH 64

typedef struct {
    int emitting;        /* current branch is active? */
    int parent_emitting; /* were we emitting before this conditional? */
    int seen_else;       /* has #else been seen? */
    int branch_taken;    /* has any branch been selected? */
} CondFrame;

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
    char  *name;
    char **params;      /* NULL for object-like; owned array of owned strings */
    int    param_count; /* -1 = object-like, >= 0 = function-like           */
    int    is_variadic; /* 1 if last param is ..., 0 otherwise               */
    char  *replacement;
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
        if (t->defs[i].params) {
            for (int j = 0; j < t->defs[i].param_count; j++)
                free(t->defs[i].params[j]);
            free(t->defs[i].params);
        }
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

/* params and param_count describe the parameter list:
 *   param_count == -1, params == NULL  → object-like
 *   param_count >= 0                   → function-like (params owned)
 * is_variadic == 1 means the macro accepts extra arguments via __VA_ARGS__.
 * On compatible redefinition, incoming params are freed. */
static void macro_define(MacroTable *t, const char *name, size_t nlen,
                          char **params, int param_count, int is_variadic,
                          const char *repl, size_t rlen) {
    MacroDef *existing = macro_find(t, name, nlen);
    if (existing) {
        /* free any incoming params regardless of outcome */
        if (params) {
            for (int i = 0; i < param_count; i++) free(params[i]);
            free(params);
        }
        if (strlen(existing->replacement) == rlen &&
            strncmp(existing->replacement, repl, rlen) == 0 &&
            existing->is_variadic == is_variadic)
            return; /* compatible redefinition */
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
    t->defs[t->count].params      = params; /* takes ownership */
    t->defs[t->count].param_count = param_count;
    t->defs[t->count].is_variadic = is_variadic;
    t->count++;
}

static void macro_undef(MacroTable *t, const char *name, size_t len) {
    for (size_t i = 0; i < t->count; i++) {
        if (strlen(t->defs[i].name) == len &&
            strncmp(t->defs[i].name, name, len) == 0) {
            free(t->defs[i].name);
            free(t->defs[i].replacement);
            if (t->defs[i].params) {
                for (int j = 0; j < t->defs[i].param_count; j++)
                    free(t->defs[i].params[j]);
                free(t->defs[i].params);
            }
            t->defs[i] = t->defs[--t->count];
            return;
        }
    }
    /* no-op if macro is not defined */
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

/* ---- Forward declarations -------------------------------------------- */

static char *preprocess_internal(const char *source, const char *source_path,
                                  int depth, MacroTable *macros,
                                  const char **include_dirs, int n_include_dirs);
static char *expand_macros_text(const char *text, MacroTable *macros);

/* ---- Read and recursively preprocess an included file ---------------- */

static char *preprocess_file(const char *path, int depth, MacroTable *macros,
                              const char **include_dirs, int n_include_dirs) {
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
    char *result = preprocess_internal(buf, path, depth, macros,
                                       include_dirs, n_include_dirs);
    free(buf);
    return result;
}

/* ---- Resolve include filenames to absolute paths --------------------- */

/* Search only -I directories in order (angle-bracket includes).
 * Returns a heap-allocated path on success, or NULL if not found. */
static char *resolve_angle_include_path(const char *fname, size_t fname_len,
                                         const char **include_dirs, int n_include_dirs) {
    for (int i = 0; i < n_include_dirs; i++) {
        size_t idir_len = strlen(include_dirs[i]);
        char *path = malloc(idir_len + 1 + fname_len + 1);
        if (!path) { fprintf(stderr, "error: out of memory\n"); exit(1); }
        memcpy(path, include_dirs[i], idir_len);
        path[idir_len] = '/';
        memcpy(path + idir_len + 1, fname, fname_len);
        path[idir_len + 1 + fname_len] = '\0';
        FILE *f = fopen(path, "r");
        if (f) { fclose(f); return path; }
        free(path);
    }
    return NULL;
}

/* Tries source file's directory first, then each -I directory in order.
 * Returns a heap-allocated path on success, or NULL if not found. */
static char *resolve_include_path(const char *fname, size_t fname_len,
                                   const char *source_path,
                                   const char **include_dirs, int n_include_dirs) {
    /* 1. Directory of the including file. */
    char *dir = get_dir(source_path);
    size_t dir_len = strlen(dir);
    char *path = malloc(dir_len + 1 + fname_len + 1);
    if (!path) { fprintf(stderr, "error: out of memory\n"); exit(1); }
    memcpy(path, dir, dir_len);
    path[dir_len] = '/';
    memcpy(path + dir_len + 1, fname, fname_len);
    path[dir_len + 1 + fname_len] = '\0';
    free(dir);
    FILE *f = fopen(path, "r");
    if (f) { fclose(f); return path; }
    free(path);

    /* 2. -I directories in command-line order. */
    for (int i = 0; i < n_include_dirs; i++) {
        size_t idir_len = strlen(include_dirs[i]);
        path = malloc(idir_len + 1 + fname_len + 1);
        if (!path) { fprintf(stderr, "error: out of memory\n"); exit(1); }
        memcpy(path, include_dirs[i], idir_len);
        path[idir_len] = '/';
        memcpy(path + idir_len + 1, fname, fname_len);
        path[idir_len + 1 + fname_len] = '\0';
        f = fopen(path, "r");
        if (f) { fclose(f); return path; }
        free(path);
    }

    return NULL;
}

/* ---- Argument collection for function-like macro calls --------------- */

/* Collects arguments from s[*pos..], which must be positioned immediately
 * after the opening '('.  Advances *pos past the closing ')'.
 * Returns args and lens arrays (caller frees elements and arrays). */
static void collect_args(const char *s, size_t *pos,
                          char ***args_out, size_t **arg_lens_out, int *count_out) {
    size_t in = *pos;
    size_t cap = 4;
    char   **args = malloc(cap * sizeof(char *));
    size_t  *lens = malloc(cap * sizeof(size_t));
    if (!args || !lens) { fprintf(stderr, "error: out of memory\n"); exit(1); }
    int count = 0;

    /* skip leading whitespace (including newlines after line splicing) */
    while (s[in] == ' ' || s[in] == '\t' || s[in] == '\n') in++;

    if (s[in] == ')') {
        /* empty argument list */
        in++;
        *pos = in;
        *args_out = args;
        *arg_lens_out = lens;
        *count_out = 0;
        return;
    }

    GrowBuf arg;
    gbuf_init(&arg, 16);
    int depth = 0;

    while (s[in]) {
        char c = s[in];

        /* string literal: pass through verbatim, commas/parens inside don't count */
        if (c == '"') {
            gbuf_push(&arg, s[in++]);
            while (s[in] && s[in] != '"' && s[in] != '\n') {
                if (s[in] == '\\' && s[in + 1]) gbuf_push(&arg, s[in++]);
                gbuf_push(&arg, s[in++]);
            }
            if (s[in] == '"') gbuf_push(&arg, s[in++]);
            continue;
        }

        /* character literal */
        if (c == '\'') {
            gbuf_push(&arg, s[in++]);
            while (s[in] && s[in] != '\'' && s[in] != '\n') {
                if (s[in] == '\\' && s[in + 1]) gbuf_push(&arg, s[in++]);
                gbuf_push(&arg, s[in++]);
            }
            if (s[in] == '\'') gbuf_push(&arg, s[in++]);
            continue;
        }

        if (c == '(') {
            depth++;
            gbuf_push(&arg, c);
            in++;
        } else if (c == ')' && depth > 0) {
            depth--;
            gbuf_push(&arg, c);
            in++;
        } else if (c == ')') {
            /* depth == 0: close of argument list */
            in++;
            /* trim trailing whitespace */
            while (arg.len > 0 &&
                   (arg.data[arg.len - 1] == ' ' || arg.data[arg.len - 1] == '\t' ||
                    arg.data[arg.len - 1] == '\n'))
                arg.len--;
            gbuf_push(&arg, '\0');
            if ((size_t)count == cap) {
                cap *= 2;
                args = realloc(args, cap * sizeof(char *));
                lens = realloc(lens, cap * sizeof(size_t));
                if (!args || !lens) { fprintf(stderr, "error: out of memory\n"); exit(1); }
            }
            size_t alen = arg.len - 1;
            args[count] = strdup(arg.data);
            lens[count] = alen;
            count++;
            free(arg.data);
            break;
        } else if (c == ',' && depth == 0) {
            /* argument separator */
            while (arg.len > 0 &&
                   (arg.data[arg.len - 1] == ' ' || arg.data[arg.len - 1] == '\t' ||
                    arg.data[arg.len - 1] == '\n'))
                arg.len--;
            gbuf_push(&arg, '\0');
            if ((size_t)count == cap) {
                cap *= 2;
                args = realloc(args, cap * sizeof(char *));
                lens = realloc(lens, cap * sizeof(size_t));
                if (!args || !lens) { fprintf(stderr, "error: out of memory\n"); exit(1); }
            }
            size_t alen = arg.len - 1;
            args[count] = strdup(arg.data);
            lens[count] = alen;
            count++;
            free(arg.data);
            gbuf_init(&arg, 16);
            in++;
            /* skip leading whitespace for the next argument */
            while (s[in] == ' ' || s[in] == '\t' || s[in] == '\n') in++;
        } else {
            gbuf_push(&arg, c);
            in++;
        }
    }

    *pos = in;
    *args_out = args;
    *arg_lens_out = lens;
    *count_out = count;
}

/* ---- Stringify a raw argument (# operator) --------------------------- */

static char *stringify_arg(const char *arg, size_t arg_len) {
    GrowBuf inner;
    gbuf_init(&inner, arg_len * 2 + 1);

    int in_space = 1; /* start true to trim leading whitespace */
    size_t last_non_space = 0;

    for (size_t i = 0; i < arg_len; i++) {
        char c = arg[i];
        if (c == ' ' || c == '\t') {
            if (!in_space) {
                gbuf_push(&inner, ' ');
                in_space = 1;
            }
        } else if (c == '"') {
            gbuf_push(&inner, '\\');
            gbuf_push(&inner, '"');
            in_space = 0;
            last_non_space = inner.len;
        } else if (c == '\\') {
            gbuf_push(&inner, '\\');
            gbuf_push(&inner, '\\');
            in_space = 0;
            last_non_space = inner.len;
        } else {
            gbuf_push(&inner, c);
            in_space = 0;
            last_non_space = inner.len;
        }
    }

    GrowBuf out;
    gbuf_init(&out, last_non_space + 3);
    gbuf_push(&out, '"');
    gbuf_append(&out, inner.data, last_non_space);
    free(inner.data);
    gbuf_push(&out, '"');
    gbuf_push(&out, '\0');
    return out.data;
}

/* ---- Substitute parameters in replacement text ----------------------- */

/* raw_args/raw_arg_lens hold the unexpanded argument text used by # stringification
 * and ## token pasting (right operand uses unexpanded form per C99). */
static char *substitute_args(const char *repl,
                              char **params, int param_count,
                              char **args, const size_t *arg_lens,
                              char **raw_args, const size_t *raw_arg_lens) {
    GrowBuf out;
    gbuf_init(&out, strlen(repl) * 2 + 1);
    const char *r = repl;
    int paste_next = 0; /* set after ##: next param uses raw (unexpanded) text */
    while (*r) {
        /* String literal in replacement: pass through verbatim. */
        if (*r == '"') {
            gbuf_push(&out, *r++);
            while (*r && *r != '"' && *r != '\n') {
                if (*r == '\\' && *(r + 1)) gbuf_push(&out, *r++);
                gbuf_push(&out, *r++);
            }
            if (*r == '"') gbuf_push(&out, *r++);
            paste_next = 0;
            continue;
        }
        /* Character literal in replacement: pass through verbatim. */
        if (*r == '\'') {
            gbuf_push(&out, *r++);
            while (*r && *r != '\'' && *r != '\n') {
                if (*r == '\\' && *(r + 1)) gbuf_push(&out, *r++);
                gbuf_push(&out, *r++);
            }
            if (*r == '\'') gbuf_push(&out, *r++);
            paste_next = 0;
            continue;
        }
        /* Token paste operator: ## */
        if (*r == '#' && *(r + 1) == '#') {
            /* Strip trailing whitespace from whatever has been emitted so far */
            while (out.len > 0 &&
                   (out.data[out.len - 1] == ' ' || out.data[out.len - 1] == '\t'))
                out.len--;
            r += 2;
            while (*r == ' ' || *r == '\t') r++;
            paste_next = 1;
            continue;
        }
        /* Stringification operator: #param */
        if (*r == '#') {
            r++;
            while (*r == ' ' || *r == '\t') r++;
            const char *id_start = r;
            while (*r && (isalnum((unsigned char)*r) || *r == '_')) r++;
            size_t id_len = (size_t)(r - id_start);
            int found = 0;
            for (int i = 0; i < param_count; i++) {
                if (strlen(params[i]) == id_len &&
                    strncmp(params[i], id_start, id_len) == 0) {
                    char *str = stringify_arg(raw_args[i], raw_arg_lens[i]);
                    gbuf_append(&out, str, strlen(str));
                    free(str);
                    found = 1;
                    break;
                }
            }
            if (!found) {
                fprintf(stderr, "error: '#' operand is not a macro parameter\n");
                free(out.data); exit(1);
            }
            paste_next = 0;
            continue;
        }
        if (isalpha((unsigned char)*r) || *r == '_') {
            const char *id_start = r;
            while (*r && (isalnum((unsigned char)*r) || *r == '_')) r++;
            size_t id_len = (size_t)(r - id_start);
            int found = 0;
            for (int i = 0; i < param_count; i++) {
                if (strlen(params[i]) == id_len &&
                    strncmp(params[i], id_start, id_len) == 0) {
                    /* Right operand of ## uses unexpanded (raw) argument text */
                    const char *text = paste_next ? raw_args[i] : args[i];
                    size_t      tlen = paste_next ? raw_arg_lens[i] : arg_lens[i];
                    gbuf_append(&out, text, tlen);
                    found = 1;
                    break;
                }
            }
            if (!found) gbuf_append(&out, id_start, id_len);
            paste_next = 0;
        } else {
            gbuf_push(&out, *r++);
            paste_next = 0;
        }
    }
    gbuf_push(&out, '\0');
    return out.data;
}

/* ---- Expand macro invocations in a plain text string ----------------- */

/* Handles identifiers (object-like and function-like macros), string and
 * character literals (passed through verbatim).  Used to pre-expand
 * function-like macro arguments and to rescan substituted replacement text. */
static char *expand_macros_text(const char *text, MacroTable *macros) {
    const char *s = text;
    GrowBuf out;
    gbuf_init(&out, strlen(s) + 1);
    size_t in = 0;

    while (s[in]) {
        char c = s[in];

        /* string literal: pass through without expanding macros inside */
        if (c == '"') {
            gbuf_push(&out, s[in++]);
            while (s[in] && s[in] != '"' && s[in] != '\n') {
                if (s[in] == '\\' && s[in + 1]) gbuf_push(&out, s[in++]);
                gbuf_push(&out, s[in++]);
            }
            if (s[in] == '"') gbuf_push(&out, s[in++]);
            continue;
        }

        /* character literal */
        if (c == '\'') {
            gbuf_push(&out, s[in++]);
            while (s[in] && s[in] != '\'' && s[in] != '\n') {
                if (s[in] == '\\' && s[in + 1]) gbuf_push(&out, s[in++]);
                gbuf_push(&out, s[in++]);
            }
            if (s[in] == '\'') gbuf_push(&out, s[in++]);
            continue;
        }

        /* identifier: check for macro */
        if (isalpha((unsigned char)c) || c == '_') {
            size_t id_start = in;
            while (s[in] && (isalnum((unsigned char)s[in]) || s[in] == '_'))
                in++;
            size_t id_len = in - id_start;
            MacroDef *def = macro_find(macros, s + id_start, id_len);
            if (def && def->param_count == -1) {
                /* object-like macro */
                gbuf_append(&out, def->replacement, strlen(def->replacement));
            } else if (def && def->param_count >= 0) {
                /* function-like macro: look ahead for '(' */
                size_t peek = in;
                while (s[peek] == ' ' || s[peek] == '\t') peek++;
                if (s[peek] == '(') {
                    in = peek + 1; /* skip whitespace and '(' */
                    char  **args     = NULL;
                    size_t *arg_lens = NULL;
                    int     arg_count = 0;
                    collect_args(s, &in, &args, &arg_lens, &arg_count);
                    if (def->is_variadic ? arg_count < def->param_count
                                        : arg_count != def->param_count) {
                        fprintf(stderr,
                                "error: argument count mismatch for macro '%.*s':"
                                " expected %s%d, got %d\n",
                                (int)id_len, s + id_start,
                                def->is_variadic ? "at least " : "",
                                def->param_count, arg_count);
                        for (int i = 0; i < arg_count; i++) free(args[i]);
                        free(args); free(arg_lens); free(out.data);
                        exit(1);
                    }
                    /* save raw args before expansion (for # stringification) */
                    char  **raw_args     = malloc(((size_t)arg_count + 1) * sizeof(char *));
                    size_t *raw_arg_lens = malloc(((size_t)arg_count + 1) * sizeof(size_t));
                    if (!raw_args || !raw_arg_lens) { fprintf(stderr, "error: out of memory\n"); exit(1); }
                    for (int i = 0; i < arg_count; i++) {
                        raw_args[i]     = strdup(args[i]);
                        raw_arg_lens[i] = arg_lens[i];
                    }
                    /* pre-expand each argument before substitution */
                    for (int i = 0; i < arg_count; i++) {
                        char *ea = expand_macros_text(args[i], macros);
                        free(args[i]);
                        args[i]     = ea;
                        arg_lens[i] = strlen(ea);
                    }
                    /* for variadic macros, build __VA_ARGS__ and extend param/arg arrays */
                    char  **subst_params    = def->params;
                    char  **subst_args      = args;
                    size_t *subst_arg_lens  = arg_lens;
                    char  **subst_raw       = raw_args;
                    size_t *subst_raw_lens  = raw_arg_lens;
                    int     subst_count     = def->param_count;
                    char   *va_exp = NULL, *va_raw = NULL;
                    char  **ext_params = NULL, **ext_args = NULL, **ext_raw = NULL;
                    size_t *ext_lens = NULL, *ext_raw_lens = NULL;
                    if (def->is_variadic) {
                        GrowBuf vab; gbuf_init(&vab, 64);
                        for (int i = def->param_count; i < arg_count; i++) {
                            if (i > def->param_count) gbuf_append(&vab, ", ", 2);
                            gbuf_append(&vab, args[i], arg_lens[i]);
                        }
                        gbuf_push(&vab, '\0');
                        va_exp = vab.data;
                        GrowBuf vrb; gbuf_init(&vrb, 64);
                        for (int i = def->param_count; i < arg_count; i++) {
                            if (i > def->param_count) gbuf_append(&vrb, ", ", 2);
                            gbuf_append(&vrb, raw_args[i], raw_arg_lens[i]);
                        }
                        gbuf_push(&vrb, '\0');
                        va_raw = vrb.data;
                        subst_count = def->param_count + 1;
                        ext_params   = malloc((size_t)subst_count * sizeof(char *));
                        ext_args     = malloc((size_t)subst_count * sizeof(char *));
                        ext_lens     = malloc((size_t)subst_count * sizeof(size_t));
                        ext_raw      = malloc((size_t)subst_count * sizeof(char *));
                        ext_raw_lens = malloc((size_t)subst_count * sizeof(size_t));
                        if (!ext_params||!ext_args||!ext_lens||!ext_raw||!ext_raw_lens) {
                            fprintf(stderr, "error: out of memory\n"); exit(1);
                        }
                        for (int i = 0; i < def->param_count; i++) {
                            ext_params[i]   = def->params[i];
                            ext_args[i]     = args[i];
                            ext_lens[i]     = arg_lens[i];
                            ext_raw[i]      = raw_args[i];
                            ext_raw_lens[i] = raw_arg_lens[i];
                        }
                        ext_params[def->param_count]   = "__VA_ARGS__";
                        ext_args[def->param_count]     = va_exp;
                        ext_lens[def->param_count]     = strlen(va_exp);
                        ext_raw[def->param_count]      = va_raw;
                        ext_raw_lens[def->param_count] = strlen(va_raw);
                        subst_params   = ext_params;
                        subst_args     = ext_args;
                        subst_arg_lens = ext_lens;
                        subst_raw      = ext_raw;
                        subst_raw_lens = ext_raw_lens;
                    }
                    /* substitute args into replacement */
                    char *subst = substitute_args(def->replacement,
                                                   subst_params, subst_count,
                                                   subst_args, subst_arg_lens,
                                                   subst_raw, subst_raw_lens);
                    /* rescan the substituted text */
                    char *rescanned = expand_macros_text(subst, macros);
                    gbuf_append(&out, rescanned, strlen(rescanned));
                    free(rescanned); free(subst);
                    for (int i = 0; i < arg_count; i++) { free(args[i]); free(raw_args[i]); }
                    free(args); free(arg_lens); free(raw_args); free(raw_arg_lens);
                    free(va_exp); free(va_raw);
                    free(ext_params); free(ext_args); free(ext_lens);
                    free(ext_raw); free(ext_raw_lens);
                } else {
                    /* not followed by '(' — emit name unchanged */
                    gbuf_append(&out, s + id_start, id_len);
                }
            } else {
                gbuf_append(&out, s + id_start, id_len);
            }
            continue;
        }

        gbuf_push(&out, s[in++]);
    }

    gbuf_push(&out, '\0');
    return out.data;
}

/* ---- Preprocessor condition expression evaluator --------------------- */

static long eval_cond_expr(const char *s, size_t *in, MacroTable *macros,
                            char *out_data, char *spliced_buf);
static long eval_cond_unary(const char *s, size_t *in, MacroTable *macros,
                             char *out_data, char *spliced_buf);
static long eval_cond_multiplicative(const char *s, size_t *in, MacroTable *macros,
                                      char *out_data, char *spliced_buf);
static long eval_cond_additive(const char *s, size_t *in, MacroTable *macros,
                                char *out_data, char *spliced_buf);
static long eval_cond_shift(const char *s, size_t *in, MacroTable *macros,
                             char *out_data, char *spliced_buf);
static long eval_cond_bitwise_and(const char *s, size_t *in, MacroTable *macros,
                                   char *out_data, char *spliced_buf);
static long eval_cond_bitwise_xor(const char *s, size_t *in, MacroTable *macros,
                                   char *out_data, char *spliced_buf);
static long eval_cond_bitwise_or(const char *s, size_t *in, MacroTable *macros,
                                  char *out_data, char *spliced_buf);

/* Evaluate the primary of a preprocessor condition: defined(...), an integer
 * literal, or an object-like macro identifier that expands to one.
 * Advances *in past the consumed tokens.  Frees out_data and spliced_buf
 * before calling exit(1) on error (matching the caller's cleanup pattern). */
static long eval_cond_primary(const char *s, size_t *in, MacroTable *macros,
                               char *out_data, char *spliced_buf) {
    while (s[*in] == ' ' || s[*in] == '\t') (*in)++;

    if (strncmp(s + *in, "defined", 7) == 0 &&
        !isalnum((unsigned char)s[*in + 7]) && s[*in + 7] != '_') {
        *in += 7;
        while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
        if (s[*in] == '(') {
            (*in)++;
            while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
            size_t name_start = *in;
            while (s[*in] && (isalnum((unsigned char)s[*in]) || s[*in] == '_'))
                (*in)++;
            size_t name_len = *in - name_start;
            while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
            if (s[*in] != ')') {
                fprintf(stderr, "error: expected ')' in defined(...)\n");
                free(out_data); free(spliced_buf); exit(1);
            }
            (*in)++;
            return macro_find(macros, s + name_start, name_len) != NULL ? 1L : 0L;
        } else if (isalpha((unsigned char)s[*in]) || s[*in] == '_') {
            size_t name_start = *in;
            while (s[*in] && (isalnum((unsigned char)s[*in]) || s[*in] == '_'))
                (*in)++;
            size_t name_len = *in - name_start;
            return macro_find(macros, s + name_start, name_len) != NULL ? 1L : 0L;
        } else {
            fprintf(stderr, "error: expected identifier after defined\n");
            free(out_data); free(spliced_buf); exit(1);
        }
    }

    if (isdigit((unsigned char)s[*in])) {
        long value = 0;
        while (isdigit((unsigned char)s[*in]))
            value = value * 10 + (s[(*in)++] - '0');
        return value;
    }

    if (isalpha((unsigned char)s[*in]) || s[*in] == '_') {
        size_t name_start = *in;
        while (s[*in] && (isalnum((unsigned char)s[*in]) || s[*in] == '_'))
            (*in)++;
        size_t name_len = *in - name_start;
        MacroDef *m = macro_find(macros, s + name_start, name_len);
        if (!m) return 0L;  /* undefined identifier evaluates to 0 */
        if (m->param_count != -1) {
            fprintf(stderr, "error: unsupported #if expression\n");
            free(out_data); free(spliced_buf); exit(1);
        }
        const char *repl = m->replacement;
        while (*repl == ' ' || *repl == '\t') repl++;
        int neg = 0;
        if (*repl == '-') { neg = 1; repl++; while (*repl == ' ' || *repl == '\t') repl++; }
        if (!isdigit((unsigned char)*repl)) {
            fprintf(stderr, "error: macro in #if does not expand to an integer\n");
            free(out_data); free(spliced_buf); exit(1);
        }
        long value = 0;
        while (isdigit((unsigned char)*repl))
            value = value * 10 + (*repl++ - '0');
        return neg ? -value : value;
    }

    if (s[*in] == '(') {
        (*in)++;
        while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
        long value = eval_cond_expr(s, in, macros, out_data, spliced_buf);
        while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
        if (s[*in] != ')') {
            fprintf(stderr, "error: expected ')' in preprocessor expression\n");
            free(out_data); free(spliced_buf); exit(1);
        }
        (*in)++;
        return value;
    }

    fprintf(stderr, "error: #if requires an integer constant or defined(...)\n");
    free(out_data); free(spliced_buf); exit(1);
}

/* Unary expression: optional leading !, -, +, ~ (chained) then primary. */
static long eval_cond_unary(const char *s, size_t *in, MacroTable *macros,
                             char *out_data, char *spliced_buf) {
    char ops[32];
    int  nops = 0;

    while (s[*in] == '!' || s[*in] == '-' || s[*in] == '+' || s[*in] == '~') {
        ops[nops++] = s[(*in)++];
        while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
    }

    long value = eval_cond_primary(s, in, macros, out_data, spliced_buf);

    for (int i = nops - 1; i >= 0; i--) {
        if      (ops[i] == '!') value = (value == 0) ? 1L : 0L;
        else if (ops[i] == '-') value = -value;
        else if (ops[i] == '~') value = ~value;
    }

    return value;
}

/* Multiplicative expression: unary { ("*" | "/" | "%") unary }, left-associative. */
static long eval_cond_multiplicative(const char *s, size_t *in, MacroTable *macros,
                                      char *out_data, char *spliced_buf) {
    long value = eval_cond_unary(s, in, macros, out_data, spliced_buf);

    for (;;) {
        while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
        if (s[*in] == '*') {
            (*in)++;
            while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
            long rhs = eval_cond_unary(s, in, macros, out_data, spliced_buf);
            value = value * rhs;
        } else if (s[*in] == '/') {
            (*in)++;
            while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
            long rhs = eval_cond_unary(s, in, macros, out_data, spliced_buf);
            if (rhs == 0) {
                fprintf(stderr, "error: division by zero in preprocessor expression\n");
                free(out_data); free(spliced_buf); exit(1);
            }
            value = value / rhs;
        } else if (s[*in] == '%') {
            (*in)++;
            while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
            long rhs = eval_cond_unary(s, in, macros, out_data, spliced_buf);
            if (rhs == 0) {
                fprintf(stderr, "error: modulo by zero in preprocessor expression\n");
                free(out_data); free(spliced_buf); exit(1);
            }
            value = value % rhs;
        } else {
            break;
        }
    }

    return value;
}

/* Additive expression: multiplicative { ("+" | "-") multiplicative }, left-associative. */
static long eval_cond_additive(const char *s, size_t *in, MacroTable *macros,
                                char *out_data, char *spliced_buf) {
    long value = eval_cond_multiplicative(s, in, macros, out_data, spliced_buf);

    for (;;) {
        while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
        if (s[*in] == '+') {
            (*in)++;
            while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
            long rhs = eval_cond_multiplicative(s, in, macros, out_data, spliced_buf);
            value = value + rhs;
        } else if (s[*in] == '-') {
            (*in)++;
            while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
            long rhs = eval_cond_multiplicative(s, in, macros, out_data, spliced_buf);
            value = value - rhs;
        } else {
            break;
        }
    }

    return value;
}

/* Shift expression: additive { ("<<" | ">>") additive }, left-associative. */
static long eval_cond_shift(const char *s, size_t *in, MacroTable *macros,
                             char *out_data, char *spliced_buf) {
    long value = eval_cond_additive(s, in, macros, out_data, spliced_buf);

    for (;;) {
        while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
        if (s[*in] == '<' && s[*in + 1] == '<') {
            *in += 2;
            while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
            long rhs = eval_cond_additive(s, in, macros, out_data, spliced_buf);
            value = value << rhs;
        } else if (s[*in] == '>' && s[*in + 1] == '>') {
            *in += 2;
            while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
            long rhs = eval_cond_additive(s, in, macros, out_data, spliced_buf);
            value = value >> rhs;
        } else {
            break;
        }
    }

    return value;
}

/* Relational expression: shift (<, <=, >, >=) shift, left-associative. */
static long eval_cond_relational(const char *s, size_t *in, MacroTable *macros,
                                  char *out_data, char *spliced_buf) {
    long value = eval_cond_shift(s, in, macros, out_data, spliced_buf);

    for (;;) {
        while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
        if (s[*in] == '<' && s[*in + 1] == '=') {
            *in += 2;
            while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
            long rhs = eval_cond_shift(s, in, macros, out_data, spliced_buf);
            value = (value <= rhs) ? 1L : 0L;
        } else if (s[*in] == '>' && s[*in + 1] == '=') {
            *in += 2;
            while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
            long rhs = eval_cond_shift(s, in, macros, out_data, spliced_buf);
            value = (value >= rhs) ? 1L : 0L;
        } else if (s[*in] == '<' && s[*in + 1] != '<') {
            (*in)++;
            while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
            long rhs = eval_cond_shift(s, in, macros, out_data, spliced_buf);
            value = (value < rhs) ? 1L : 0L;
        } else if (s[*in] == '>' && s[*in + 1] != '>') {
            (*in)++;
            while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
            long rhs = eval_cond_shift(s, in, macros, out_data, spliced_buf);
            value = (value > rhs) ? 1L : 0L;
        } else {
            break;
        }
    }

    return value;
}

/* Equality expression: relational (==, !=) relational, left-associative. */
static long eval_cond_equality(const char *s, size_t *in, MacroTable *macros,
                                char *out_data, char *spliced_buf) {
    long value = eval_cond_relational(s, in, macros, out_data, spliced_buf);

    for (;;) {
        while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
        if (s[*in] == '=' && s[*in + 1] == '=') {
            *in += 2;
            while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
            long rhs = eval_cond_relational(s, in, macros, out_data, spliced_buf);
            value = (value == rhs) ? 1L : 0L;
        } else if (s[*in] == '!' && s[*in + 1] == '=') {
            *in += 2;
            while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
            long rhs = eval_cond_relational(s, in, macros, out_data, spliced_buf);
            value = (value != rhs) ? 1L : 0L;
        } else {
            break;
        }
    }

    return value;
}

/* Bitwise AND expression: equality { "&" equality }, left-associative. */
static long eval_cond_bitwise_and(const char *s, size_t *in, MacroTable *macros,
                                   char *out_data, char *spliced_buf) {
    long value = eval_cond_equality(s, in, macros, out_data, spliced_buf);

    for (;;) {
        while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
        if (s[*in] == '&' && s[*in + 1] != '&') {
            (*in)++;
            while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
            long rhs = eval_cond_equality(s, in, macros, out_data, spliced_buf);
            value = value & rhs;
        } else {
            break;
        }
    }

    return value;
}

/* Bitwise XOR expression: bitwise_and { "^" bitwise_and }, left-associative. */
static long eval_cond_bitwise_xor(const char *s, size_t *in, MacroTable *macros,
                                   char *out_data, char *spliced_buf) {
    long value = eval_cond_bitwise_and(s, in, macros, out_data, spliced_buf);

    for (;;) {
        while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
        if (s[*in] == '^') {
            (*in)++;
            while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
            long rhs = eval_cond_bitwise_and(s, in, macros, out_data, spliced_buf);
            value = value ^ rhs;
        } else {
            break;
        }
    }

    return value;
}

/* Bitwise OR expression: bitwise_xor { "|" bitwise_xor }, left-associative. */
static long eval_cond_bitwise_or(const char *s, size_t *in, MacroTable *macros,
                                  char *out_data, char *spliced_buf) {
    long value = eval_cond_bitwise_xor(s, in, macros, out_data, spliced_buf);

    for (;;) {
        while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
        if (s[*in] == '|' && s[*in + 1] != '|') {
            (*in)++;
            while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
            long rhs = eval_cond_bitwise_xor(s, in, macros, out_data, spliced_buf);
            value = value | rhs;
        } else {
            break;
        }
    }

    return value;
}

/* Logical AND expression: bitwise_or { "&&" bitwise_or }, left-associative. */
static long eval_cond_logical_and(const char *s, size_t *in, MacroTable *macros,
                                   char *out_data, char *spliced_buf) {
    long value = eval_cond_bitwise_or(s, in, macros, out_data, spliced_buf);

    for (;;) {
        while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
        if (s[*in] == '&' && s[*in + 1] == '&') {
            *in += 2;
            while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
            long rhs = eval_cond_bitwise_or(s, in, macros, out_data, spliced_buf);
            value = (value && rhs) ? 1L : 0L;
        } else {
            break;
        }
    }

    return value;
}

/* Logical OR expression: logical_and { "||" logical_and }, left-associative. */
static long eval_cond_logical_or(const char *s, size_t *in, MacroTable *macros,
                                  char *out_data, char *spliced_buf) {
    long value = eval_cond_logical_and(s, in, macros, out_data, spliced_buf);

    for (;;) {
        while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
        if (s[*in] == '|' && s[*in + 1] == '|') {
            *in += 2;
            while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
            long rhs = eval_cond_logical_and(s, in, macros, out_data, spliced_buf);
            value = (value || rhs) ? 1L : 0L;
        } else {
            break;
        }
    }

    return value;
}

/* Top-level preprocessor condition expression evaluator. */
static long eval_cond_expr(const char *s, size_t *in, MacroTable *macros,
                            char *out_data, char *spliced_buf) {
    return eval_cond_logical_or(s, in, macros, out_data, spliced_buf);
}

/* ---- Phase 2: strip comments, expand directives and macros ----------- */

static char *preprocess_internal(const char *source, const char *source_path,
                                  int depth, MacroTable *macros,
                                  const char **include_dirs, int n_include_dirs) {
    if (depth > MAX_INCLUDE_DEPTH) {
        fprintf(stderr, "error: maximum include depth exceeded (possible recursive include)\n");
        exit(1);
    }

    char *spliced = splice_lines(source);
    const char *s = spliced;
    size_t slen = strlen(s);

    GrowBuf out;
    gbuf_init(&out, slen + 1);

    CondFrame cond_stack[MAX_COND_DEPTH];
    int cond_depth = 0;
    int emitting = 1;

    size_t in = 0;
    int line_has_content = 0;
    int current_line = 1;

    while (s[in]) {
        char c = s[in];

        /* Newline: reset line tracking. */
        if (c == '\n') {
            current_line++;
            line_has_content = 0;
            gbuf_push(&out, s[in++]);
            continue;
        }

        /* Whitespace: pass through, does not count as content. */
        if (isspace((unsigned char)c)) {
            if (emitting) gbuf_push(&out, s[in]);
            in++;
            continue;
        }

        /* Line comment: skip to end of line (newline is kept). */
        if (c == '/' && s[in + 1] == '/') {
            while (s[in] && s[in] != '\n') in++;
            continue;
        }

        /* Block comment: replace with a single space, preserving newlines
         * so that line numbers after the comment remain correct. */
        if (c == '/' && s[in + 1] == '*') {
            in += 2;
            while (s[in] && !(s[in] == '*' && s[in + 1] == '/')) {
                if (s[in] == '\n') {
                    current_line++;
                    gbuf_push(&out, '\n');
                }
                in++;
            }
            if (!s[in]) {
                fprintf(stderr, "error: unterminated block comment\n");
                free(out.data);
                free(spliced);
                exit(1);
            }
            in += 2;
            if (emitting) gbuf_push(&out, ' ');
            continue;
        }

        /* Preprocessor directive: '#' is first non-whitespace on line. */
        if (c == '#' && !line_has_content) {
            in++; /* skip '#' */
            while (s[in] == ' ' || s[in] == '\t') in++;

            /* #ifdef NAME */
            if (strncmp(s + in, "ifdef", 5) == 0 &&
                !isalnum((unsigned char)s[in + 5]) && s[in + 5] != '_') {
                in += 5;
                while (s[in] == ' ' || s[in] == '\t') in++;
                size_t name_start = in;
                while (s[in] && (isalnum((unsigned char)s[in]) || s[in] == '_'))
                    in++;
                size_t name_len = in - name_start;
                while (s[in] && s[in] != '\n') in++;
                if (cond_depth >= MAX_COND_DEPTH) {
                    fprintf(stderr, "error: conditional nesting too deep\n");
                    free(out.data); free(spliced); exit(1);
                }
                int is_defined = macro_find(macros, s + name_start, name_len) != NULL;
                cond_stack[cond_depth].parent_emitting = emitting;
                cond_stack[cond_depth].emitting = emitting && is_defined;
                cond_stack[cond_depth].seen_else = 0;
                cond_stack[cond_depth].branch_taken = emitting && is_defined;
                cond_depth++;
                emitting = cond_stack[cond_depth - 1].emitting;
                continue;
            }

            /* #ifndef NAME */
            if (strncmp(s + in, "ifndef", 6) == 0 &&
                !isalnum((unsigned char)s[in + 6]) && s[in + 6] != '_') {
                in += 6;
                while (s[in] == ' ' || s[in] == '\t') in++;
                size_t name_start = in;
                while (s[in] && (isalnum((unsigned char)s[in]) || s[in] == '_'))
                    in++;
                size_t name_len = in - name_start;
                while (s[in] && s[in] != '\n') in++;
                if (cond_depth >= MAX_COND_DEPTH) {
                    fprintf(stderr, "error: conditional nesting too deep\n");
                    free(out.data); free(spliced); exit(1);
                }
                int is_defined = macro_find(macros, s + name_start, name_len) != NULL;
                cond_stack[cond_depth].parent_emitting = emitting;
                cond_stack[cond_depth].emitting = emitting && !is_defined;
                cond_stack[cond_depth].seen_else = 0;
                cond_stack[cond_depth].branch_taken = emitting && !is_defined;
                cond_depth++;
                emitting = cond_stack[cond_depth - 1].emitting;
                continue;
            }

            /* #if <integer> */
            if (strncmp(s + in, "if", 2) == 0 &&
                !isalnum((unsigned char)s[in + 2]) && s[in + 2] != '_') {
                in += 2;
                if (cond_depth >= MAX_COND_DEPTH) {
                    fprintf(stderr, "error: conditional nesting too deep\n");
                    free(out.data); free(spliced); exit(1);
                }
                int cond_val = 0;
                if (emitting) {
                    while (s[in] == ' ' || s[in] == '\t') in++;
                    long ifval = eval_cond_expr(s, &in, macros, out.data, spliced);
                    while (s[in] == ' ' || s[in] == '\t') in++;
                    if (s[in] != '\n' && s[in] != '\0') {
                        fprintf(stderr, "error: extra tokens after #if expression\n");
                        free(out.data); free(spliced); exit(1);
                    }
                    cond_val = (ifval != 0);
                }
                while (s[in] && s[in] != '\n') in++;
                cond_stack[cond_depth].parent_emitting = emitting;
                cond_stack[cond_depth].emitting = emitting && cond_val;
                cond_stack[cond_depth].seen_else = 0;
                cond_stack[cond_depth].branch_taken = emitting && cond_val;
                cond_depth++;
                emitting = cond_stack[cond_depth - 1].emitting;
                continue;
            }

            /* #elif <integer> */
            if (strncmp(s + in, "elif", 4) == 0 &&
                !isalnum((unsigned char)s[in + 4]) && s[in + 4] != '_') {
                in += 4;
                if (cond_depth == 0) {
                    fprintf(stderr, "error: #elif without conditional\n");
                    free(out.data); free(spliced); exit(1);
                }
                CondFrame *top = &cond_stack[cond_depth - 1];
                if (top->seen_else) {
                    fprintf(stderr, "error: #elif after else\n");
                    free(out.data); free(spliced); exit(1);
                }
                int cond_val = 0;
                if (top->parent_emitting && !top->branch_taken) {
                    while (s[in] == ' ' || s[in] == '\t') in++;
                    long elifval = eval_cond_expr(s, &in, macros, out.data, spliced);
                    while (s[in] == ' ' || s[in] == '\t') in++;
                    if (s[in] != '\n' && s[in] != '\0') {
                        fprintf(stderr, "error: extra tokens after #elif expression\n");
                        free(out.data); free(spliced); exit(1);
                    }
                    cond_val = (elifval != 0);
                }
                while (s[in] && s[in] != '\n') in++;
                if (top->parent_emitting) {
                    if (!top->branch_taken && cond_val) {
                        top->emitting = 1;
                        top->branch_taken = 1;
                    } else {
                        top->emitting = 0;
                    }
                }
                emitting = top->emitting;
                continue;
            }

            /* #else */
            if (strncmp(s + in, "else", 4) == 0 &&
                !isalnum((unsigned char)s[in + 4]) && s[in + 4] != '_') {
                in += 4;
                while (s[in] && s[in] != '\n') in++;
                if (cond_depth == 0) {
                    fprintf(stderr, "error: #else without conditional\n");
                    free(out.data); free(spliced); exit(1);
                }
                CondFrame *top = &cond_stack[cond_depth - 1];
                if (top->seen_else) {
                    fprintf(stderr, "error: duplicate else in conditional\n");
                    free(out.data); free(spliced); exit(1);
                }
                top->seen_else = 1;
                if (top->parent_emitting)
                    top->emitting = !top->branch_taken;
                emitting = top->emitting;
                continue;
            }

            /* #endif */
            if (strncmp(s + in, "endif", 5) == 0 &&
                !isalnum((unsigned char)s[in + 5]) && s[in + 5] != '_') {
                in += 5;
                while (s[in] && s[in] != '\n') in++;
                if (cond_depth == 0) {
                    fprintf(stderr, "error: #endif without conditional\n");
                    free(out.data); free(spliced); exit(1);
                }
                cond_depth--;
                emitting = cond_depth > 0 ? cond_stack[cond_depth - 1].emitting : 1;
                continue;
            }

            /* In inactive regions, skip all remaining directives without error. */
            if (!emitting) {
                while (s[in] && s[in] != '\n') in++;
                continue;
            }

            /* #include "filename" or #include <filename> */
            if (strncmp(s + in, "include", 7) == 0 &&
                !isalnum((unsigned char)s[in + 7]) && s[in + 7] != '_') {
                in += 7;
                while (s[in] == ' ' || s[in] == '\t') in++;

                int is_angle = (s[in] == '<');
                if (s[in] != '"' && s[in] != '<') {
                    fprintf(stderr, "error: expected '\"' or '<' after #include\n");
                    free(out.data);
                    free(spliced);
                    exit(1);
                }
                char close_delim = is_angle ? '>' : '"';
                in++; /* skip opening delimiter */

                size_t fname_start = in;
                while (s[in] && s[in] != close_delim && s[in] != '\n') in++;
                if (s[in] != close_delim) {
                    fprintf(stderr, "error: unterminated filename in #include\n");
                    free(out.data);
                    free(spliced);
                    exit(1);
                }
                size_t fname_len = in - fname_start;
                in++; /* skip closing delimiter */

                /* Discard rest of directive line. */
                while (s[in] && s[in] != '\n') in++;

                char *include_path;
                if (is_angle) {
                    /* Angle includes: search only -I directories. */
                    include_path = resolve_angle_include_path(s + fname_start, fname_len,
                                                              include_dirs, n_include_dirs);
                } else {
                    /* Quoted includes: source dir first, then -I dirs. */
                    include_path = resolve_include_path(s + fname_start, fname_len,
                                                        source_path,
                                                        include_dirs, n_include_dirs);
                }
                if (!include_path) {
                    char fname_buf[256];
                    size_t copy = fname_len < sizeof(fname_buf) - 1
                                  ? fname_len : sizeof(fname_buf) - 1;
                    memcpy(fname_buf, s + fname_start, copy);
                    fname_buf[copy] = '\0';
                    if (is_angle)
                        fprintf(stderr, "error: include file not found: <%s>\n", fname_buf);
                    else
                        fprintf(stderr, "error: include file not found: \"%s\"\n", fname_buf);
                    free(out.data); free(spliced); exit(1);
                }

                char *included = preprocess_file(include_path, depth + 1, macros,
                                                  include_dirs, n_include_dirs);
                /* Inject enter-file marker: \x01<1>:<include_path>\n */
                char loc_marker[512];
                int mlen = snprintf(loc_marker, sizeof(loc_marker),
                                    "\x01" "1:%s\n", include_path);
                if (mlen > 0 && (size_t)mlen < sizeof(loc_marker))
                    gbuf_append(&out, loc_marker, (size_t)mlen);
                free(include_path);
                gbuf_append(&out, included, strlen(included));
                free(included);
                /* Inject return-to-parent marker: \x01<current_line>:<source_path>\n
                 * The \n from the #include directive line (still in the preprocessed
                 * output after this handler returns) will advance the line count by 1,
                 * landing on the correct next line of the parent file. */
                mlen = snprintf(loc_marker, sizeof(loc_marker),
                                "\x01" "%d:%s\n",
                                current_line,
                                source_path ? source_path : "");
                if (mlen > 0 && (size_t)mlen < sizeof(loc_marker))
                    gbuf_append(&out, loc_marker, (size_t)mlen);
                continue;
            }

            /* #define NAME[(params)] [replacement] */
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

                /* Determine if function-like: '(' immediately after name (no space). */
                char  **params      = NULL;
                int     param_count = -1; /* -1 = object-like */
                int     is_variadic = 0;

                if (s[in] == '(') {
                    in++; /* skip '(' */
                    param_count = 0;
                    size_t params_cap = 4;
                    params = malloc(params_cap * sizeof(char *));
                    if (!params) { fprintf(stderr, "error: out of memory\n"); exit(1); }

                    /* skip whitespace */
                    while (s[in] == ' ' || s[in] == '\t') in++;

                    if (s[in] == ')') {
                        /* zero-parameter macro */
                        in++;
                    } else if (s[in] == '.' && s[in+1] == '.' && s[in+2] == '.') {
                        /* #define M(...) — no fixed params, variadic only */
                        in += 3;
                        is_variadic = 1;
                        while (s[in] == ' ' || s[in] == '\t') in++;
                        if (s[in] != ')') {
                            fprintf(stderr,
                                    "error: expected ')' after '...' in macro parameter list\n");
                            free(params); free(out.data); free(spliced); exit(1);
                        }
                        in++;
                    } else {
                        /* parse parameter name list */
                        while (1) {
                            if (!isalpha((unsigned char)s[in]) && s[in] != '_') {
                                fprintf(stderr,
                                        "error: expected parameter name in macro definition\n");
                                for (int i = 0; i < param_count; i++) free(params[i]);
                                free(params);
                                free(out.data); free(spliced);
                                exit(1);
                            }
                            size_t pstart = in;
                            while (s[in] && (isalnum((unsigned char)s[in]) || s[in] == '_'))
                                in++;
                            size_t plen = in - pstart;
                            char *pname = malloc(plen + 1);
                            if (!pname) { fprintf(stderr, "error: out of memory\n"); exit(1); }
                            memcpy(pname, s + pstart, plen);
                            pname[plen] = '\0';

                            if ((size_t)param_count == params_cap) {
                                params_cap *= 2;
                                params = realloc(params, params_cap * sizeof(char *));
                                if (!params) { fprintf(stderr, "error: out of memory\n"); exit(1); }
                            }
                            params[param_count++] = pname;

                            while (s[in] == ' ' || s[in] == '\t') in++;
                            if (s[in] == ')') { in++; break; }
                            if (s[in] == ',') {
                                in++;
                                while (s[in] == ' ' || s[in] == '\t') in++;
                                /* check for '...' after comma */
                                if (s[in] == '.' && s[in+1] == '.' && s[in+2] == '.') {
                                    in += 3;
                                    is_variadic = 1;
                                    while (s[in] == ' ' || s[in] == '\t') in++;
                                    if (s[in] != ')') {
                                        fprintf(stderr,
                                                "error: expected ')' after '...' in macro parameter list\n");
                                        for (int i = 0; i < param_count; i++) free(params[i]);
                                        free(params); free(out.data); free(spliced); exit(1);
                                    }
                                    in++;
                                    break;
                                }
                                continue;
                            }
                            fprintf(stderr,
                                    "error: expected ',' or ')' in macro parameter list\n");
                            for (int i = 0; i < param_count; i++) free(params[i]);
                            free(params);
                            free(out.data); free(spliced);
                            exit(1);
                        }
                    }
                }

                /* Skip whitespace separating name/params from replacement. */
                while (s[in] == ' ' || s[in] == '\t') in++;

                /* Replacement: rest of line, trailing whitespace trimmed. */
                size_t repl_start = in;
                while (s[in] && s[in] != '\n') in++;
                size_t repl_end = in;
                while (repl_end > repl_start &&
                       (s[repl_end - 1] == ' ' || s[repl_end - 1] == '\t'))
                    repl_end--;

                /* Validate '#' and '##' usage in replacement text. */
                for (size_t k = repl_start; k < repl_end; k++) {
                    if (s[k] == '"') {
                        k++;
                        while (k < repl_end && s[k] != '"' && s[k] != '\n') {
                            if (s[k] == '\\' && k + 1 < repl_end) k++;
                            k++;
                        }
                        continue;
                    }
                    if (s[k] == '\'') {
                        k++;
                        while (k < repl_end && s[k] != '\'' && s[k] != '\n') {
                            if (s[k] == '\\' && k + 1 < repl_end) k++;
                            k++;
                        }
                        continue;
                    }
                    if (s[k] == '#' && k + 1 < repl_end && s[k + 1] == '#') {
                        /* Token paste operator: ## */
                        if (param_count == -1) {
                            fprintf(stderr,
                                    "error: '##' in object-like macro is not permitted\n");
                            free(out.data); free(spliced); exit(1);
                        }
                        if (k == repl_start) {
                            fprintf(stderr,
                                    "error: hash hash at start of replacement list is not permitted\n");
                            for (int i = 0; i < param_count; i++) free(params[i]);
                            free(params); free(out.data); free(spliced); exit(1);
                        }
                        size_t j = k + 2;
                        while (j < repl_end && (s[j] == ' ' || s[j] == '\t')) j++;
                        if (j >= repl_end) {
                            fprintf(stderr,
                                    "error: hash hash at end of replacement list is not permitted\n");
                            for (int i = 0; i < param_count; i++) free(params[i]);
                            free(params); free(out.data); free(spliced); exit(1);
                        }
                        k++; /* skip second '#'; outer loop increments past both */
                        continue;
                    }
                    if (s[k] == '#') {
                        if (param_count == -1) {
                            fprintf(stderr,
                                    "error: hash in object like macro is not permitted\n");
                            free(out.data); free(spliced); exit(1);
                        }
                        size_t j = k + 1;
                        while (j < repl_end && (s[j] == ' ' || s[j] == '\t')) j++;
                        if (!isalpha((unsigned char)s[j]) && s[j] != '_') {
                            fprintf(stderr,
                                    "error: bare hash in replacement list\n");
                            for (int i = 0; i < param_count; i++) free(params[i]);
                            free(params); free(out.data); free(spliced); exit(1);
                        }
                        size_t pstart = j;
                        while (j < repl_end && (isalnum((unsigned char)s[j]) || s[j] == '_'))
                            j++;
                        size_t plen = j - pstart;
                        int found_param = 0;
                        for (int pi = 0; pi < param_count; pi++) {
                            if (strlen(params[pi]) == plen &&
                                strncmp(params[pi], s + pstart, plen) == 0) {
                                found_param = 1;
                                break;
                            }
                        }
                        /* __VA_ARGS__ is valid in variadic macro replacement */
                        if (!found_param && is_variadic &&
                            plen == 11 && strncmp(s + pstart, "__VA_ARGS__", 11) == 0)
                            found_param = 1;
                        if (!found_param) {
                            fprintf(stderr,
                                    "error: hash not followed by param '%.*s'\n",
                                    (int)plen, s + pstart);
                            for (int i = 0; i < param_count; i++) free(params[i]);
                            free(params); free(out.data); free(spliced); exit(1);
                        }
                    }
                }

                macro_define(macros,
                             s + name_start, name_len,
                             params, param_count, is_variadic,
                             s + repl_start, repl_end - repl_start);
                /* Directive line consumed; newline handled on next iteration. */
                continue;
            }

            /* #undef NAME */
            if (strncmp(s + in, "undef", 5) == 0 &&
                !isalnum((unsigned char)s[in + 5]) && s[in + 5] != '_') {
                in += 5;
                while (s[in] == ' ' || s[in] == '\t') in++;
                size_t name_start = in;
                while (s[in] && (isalnum((unsigned char)s[in]) || s[in] == '_'))
                    in++;
                size_t name_len = in - name_start;
                while (s[in] && s[in] != '\n') in++;
                if (name_len > 0)
                    macro_undef(macros, s + name_start, name_len);
                continue;
            }

            /* #error message */
            if (strncmp(s + in, "error", 5) == 0 &&
                !isalnum((unsigned char)s[in + 5]) && s[in + 5] != '_') {
                in += 5;
                while (s[in] == ' ' || s[in] == '\t') in++;
                size_t msg_start = in;
                while (s[in] && s[in] != '\n') in++;
                size_t msg_len = in - msg_start;
                while (msg_len > 0 &&
                       (s[msg_start + msg_len - 1] == ' ' ||
                        s[msg_start + msg_len - 1] == '\t'))
                    msg_len--;
                fprintf(stderr, "error: #error %.*s\n", (int)msg_len, s + msg_start);
                free(out.data); free(spliced); exit(1);
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
            if (emitting) gbuf_push(&out, s[in]);
            in++;
            while (s[in] && s[in] != '"' && s[in] != '\n') {
                if (s[in] == '\\' && s[in + 1]) {
                    if (emitting) gbuf_push(&out, s[in]);
                    in++;
                }
                if (emitting) gbuf_push(&out, s[in]);
                in++;
            }
            if (s[in] == '"') {
                if (emitting) gbuf_push(&out, s[in]);
                in++;
            }
            continue;
        }

        /* Character literal: pass through unchanged. */
        if (c == '\'') {
            line_has_content = 1;
            if (emitting) gbuf_push(&out, s[in]);
            in++;
            while (s[in] && s[in] != '\'' && s[in] != '\n') {
                if (s[in] == '\\' && s[in + 1]) {
                    if (emitting) gbuf_push(&out, s[in]);
                    in++;
                }
                if (emitting) gbuf_push(&out, s[in]);
                in++;
            }
            if (s[in] == '\'') {
                if (emitting) gbuf_push(&out, s[in]);
                in++;
            }
            continue;
        }

        /* Identifier: expand if it names a macro, otherwise pass through. */
        if (isalpha((unsigned char)c) || c == '_') {
            size_t id_start = in;
            while (s[in] && (isalnum((unsigned char)s[in]) || s[in] == '_'))
                in++;
            size_t id_len = in - id_start;
            if (emitting) {
                if (id_len == 8 && strncmp(s + id_start, "__FILE__", 8) == 0) {
                    gbuf_push(&out, '"');
                    for (const char *fp = source_path; fp && *fp; fp++) {
                        if (*fp == '"' || *fp == '\\') gbuf_push(&out, '\\');
                        gbuf_push(&out, *fp);
                    }
                    gbuf_push(&out, '"');
                } else if (id_len == 8 && strncmp(s + id_start, "__LINE__", 8) == 0) {
                    char lbuf[32];
                    int llen = snprintf(lbuf, sizeof(lbuf), "%d", current_line);
                    gbuf_append(&out, lbuf, (size_t)llen);
                } else {
                    MacroDef *def = macro_find(macros, s + id_start, id_len);
                    if (def && def->param_count == -1) {
                        /* object-like macro: expand replacement directly */
                        gbuf_append(&out, def->replacement, strlen(def->replacement));
                    } else if (def && def->param_count >= 0) {
                        /* function-like macro: look ahead for '(' */
                        size_t peek = in;
                        while (s[peek] == ' ' || s[peek] == '\t') peek++;
                        if (s[peek] == '(') {
                            in = peek + 1; /* skip optional whitespace and '(' */
                            char  **args      = NULL;
                            size_t *arg_lens  = NULL;
                            int     arg_count = 0;
                            collect_args(s, &in, &args, &arg_lens, &arg_count);
                            if (def->is_variadic ? arg_count < def->param_count
                                                 : arg_count != def->param_count) {
                                fprintf(stderr,
                                        "error: argument count mismatch for macro '%.*s':"
                                        " expected %s%d, got %d\n",
                                        (int)id_len, s + id_start,
                                        def->is_variadic ? "at least " : "",
                                        def->param_count, arg_count);
                                for (int i = 0; i < arg_count; i++) free(args[i]);
                                free(args); free(arg_lens);
                                free(out.data); free(spliced);
                                exit(1);
                            }
                            /* save raw args before expansion (for # stringification) */
                            char  **raw_args     = malloc(((size_t)arg_count + 1) * sizeof(char *));
                            size_t *raw_arg_lens = malloc(((size_t)arg_count + 1) * sizeof(size_t));
                            if (!raw_args || !raw_arg_lens) { fprintf(stderr, "error: out of memory\n"); exit(1); }
                            for (int i = 0; i < arg_count; i++) {
                                raw_args[i]     = strdup(args[i]);
                                raw_arg_lens[i] = arg_lens[i];
                            }
                            /* pre-expand each argument */
                            for (int i = 0; i < arg_count; i++) {
                                char *ea = expand_macros_text(args[i], macros);
                                free(args[i]);
                                args[i]     = ea;
                                arg_lens[i] = strlen(ea);
                            }
                            /* for variadic macros, build __VA_ARGS__ and extend param/arg arrays */
                            char  **subst_params    = def->params;
                            char  **subst_args      = args;
                            size_t *subst_arg_lens  = arg_lens;
                            char  **subst_raw       = raw_args;
                            size_t *subst_raw_lens  = raw_arg_lens;
                            int     subst_count     = def->param_count;
                            char   *va_exp = NULL, *va_raw = NULL;
                            char  **ext_params = NULL, **ext_args = NULL, **ext_raw = NULL;
                            size_t *ext_lens = NULL, *ext_raw_lens = NULL;
                            if (def->is_variadic) {
                                GrowBuf vab; gbuf_init(&vab, 64);
                                for (int i = def->param_count; i < arg_count; i++) {
                                    if (i > def->param_count) gbuf_append(&vab, ", ", 2);
                                    gbuf_append(&vab, args[i], arg_lens[i]);
                                }
                                gbuf_push(&vab, '\0');
                                va_exp = vab.data;
                                GrowBuf vrb; gbuf_init(&vrb, 64);
                                for (int i = def->param_count; i < arg_count; i++) {
                                    if (i > def->param_count) gbuf_append(&vrb, ", ", 2);
                                    gbuf_append(&vrb, raw_args[i], raw_arg_lens[i]);
                                }
                                gbuf_push(&vrb, '\0');
                                va_raw = vrb.data;
                                subst_count = def->param_count + 1;
                                ext_params   = malloc((size_t)subst_count * sizeof(char *));
                                ext_args     = malloc((size_t)subst_count * sizeof(char *));
                                ext_lens     = malloc((size_t)subst_count * sizeof(size_t));
                                ext_raw      = malloc((size_t)subst_count * sizeof(char *));
                                ext_raw_lens = malloc((size_t)subst_count * sizeof(size_t));
                                if (!ext_params||!ext_args||!ext_lens||!ext_raw||!ext_raw_lens) {
                                    fprintf(stderr, "error: out of memory\n"); exit(1);
                                }
                                for (int i = 0; i < def->param_count; i++) {
                                    ext_params[i]   = def->params[i];
                                    ext_args[i]     = args[i];
                                    ext_lens[i]     = arg_lens[i];
                                    ext_raw[i]      = raw_args[i];
                                    ext_raw_lens[i] = raw_arg_lens[i];
                                }
                                ext_params[def->param_count]   = "__VA_ARGS__";
                                ext_args[def->param_count]     = va_exp;
                                ext_lens[def->param_count]     = strlen(va_exp);
                                ext_raw[def->param_count]      = va_raw;
                                ext_raw_lens[def->param_count] = strlen(va_raw);
                                subst_params   = ext_params;
                                subst_args     = ext_args;
                                subst_arg_lens = ext_lens;
                                subst_raw      = ext_raw;
                                subst_raw_lens = ext_raw_lens;
                            }
                            /* substitute into replacement, then rescan */
                            char *subst     = substitute_args(def->replacement,
                                                               subst_params, subst_count,
                                                               subst_args, subst_arg_lens,
                                                               subst_raw, subst_raw_lens);
                            char *rescanned = expand_macros_text(subst, macros);
                            gbuf_append(&out, rescanned, strlen(rescanned));
                            free(rescanned); free(subst);
                            for (int i = 0; i < arg_count; i++) { free(args[i]); free(raw_args[i]); }
                            free(args); free(arg_lens); free(raw_args); free(raw_arg_lens);
                            free(va_exp); free(va_raw);
                            free(ext_params); free(ext_args); free(ext_lens);
                            free(ext_raw); free(ext_raw_lens);
                        } else {
                            /* function-like macro not followed by '(' — pass through */
                            gbuf_append(&out, s + id_start, id_len);
                        }
                    } else {
                        gbuf_append(&out, s + id_start, id_len);
                    }
                }
            }
            line_has_content = 1;
            continue;
        }

        /* Regular character. */
        line_has_content = 1;
        if (emitting) gbuf_push(&out, s[in]);
        in++;
    }

    if (cond_depth > 0) {
        fprintf(stderr, "error: missing endif\n");
        free(out.data);
        free(spliced);
        exit(1);
    }
    gbuf_push(&out, '\0');
    free(spliced);
    return out.data;
}

char *preprocess(const char *source, const char *source_path) {
    return preprocess_with_defines(source, source_path, NULL, 0);
}

char *preprocess_with_defines(const char *source, const char *source_path,
                               const char **defines, int n_defines) {
    return preprocess_with_defines_and_includes(source, source_path,
                                                defines, n_defines, NULL, 0);
}

char *preprocess_with_defines_and_includes(const char *source,
                                            const char *source_path,
                                            const char **defines, int n_defines,
                                            const char **include_dirs,
                                            int n_include_dirs) {
    MacroTable macros;
    macro_table_init(&macros);

    /* Standard predefined macros — inserted before user -D definitions. */
    macro_define(&macros, "__STDC__",         strlen("__STDC__"),         NULL, -1, 0, "1",      strlen("1"));
    macro_define(&macros, "__STDC_VERSION__", strlen("__STDC_VERSION__"), NULL, -1, 0, "199901", strlen("199901"));
    macro_define(&macros, "__CLAUDEC99__",    strlen("__CLAUDEC99__"),    NULL, -1, 0, "1",      strlen("1"));
    macro_define(&macros, "__STDC_HOSTED__",  strlen("__STDC_HOSTED__"),  NULL, -1, 0, "1",      strlen("1"));

    /* Runtime context predefined macros — computed once at preprocessing start. */
    {
        time_t     now = time(NULL);
        struct tm *tm  = localtime(&now);
        char date_buf[16]; /* "\"Mmm DD YYYY\"\0" fits in 16 */
        char time_buf[12]; /* "\"HH:MM:SS\"\0"   fits in 12 */
        strftime(date_buf, sizeof(date_buf), "\"%b %e %Y\"", tm);
        strftime(time_buf, sizeof(time_buf), "\"%H:%M:%S\"", tm);
        macro_define(&macros, "__DATE__", strlen("__DATE__"), NULL, -1, 0, date_buf, strlen(date_buf));
        macro_define(&macros, "__TIME__", strlen("__TIME__"), NULL, -1, 0, time_buf, strlen(time_buf));
    }

    /* Target/ABI predefined macros — x86_64 Linux LP64. */
    macro_define(&macros, "__x86_64__",         strlen("__x86_64__"),         NULL, -1, 0, "1", 1);
    macro_define(&macros, "__linux__",           strlen("__linux__"),           NULL, -1, 0, "1", 1);
    macro_define(&macros, "__unix__",            strlen("__unix__"),            NULL, -1, 0, "1", 1);
    macro_define(&macros, "__LP64__",            strlen("__LP64__"),            NULL, -1, 0, "1", 1);
    macro_define(&macros, "_LP64",               strlen("_LP64"),               NULL, -1, 0, "1", 1);
    macro_define(&macros, "__CHAR_BIT__",        strlen("__CHAR_BIT__"),        NULL, -1, 0, "8", 1);
    macro_define(&macros, "__SIZEOF_CHAR__",     strlen("__SIZEOF_CHAR__"),     NULL, -1, 0, "1", 1);
    macro_define(&macros, "__SIZEOF_SHORT__",    strlen("__SIZEOF_SHORT__"),    NULL, -1, 0, "2", 1);
    macro_define(&macros, "__SIZEOF_INT__",      strlen("__SIZEOF_INT__"),      NULL, -1, 0, "4", 1);
    macro_define(&macros, "__SIZEOF_LONG__",     strlen("__SIZEOF_LONG__"),     NULL, -1, 0, "8", 1);
    macro_define(&macros, "__SIZEOF_POINTER__",   strlen("__SIZEOF_POINTER__"),   NULL, -1, 0, "8", 1);
    macro_define(&macros, "__SIZEOF_SIZE_T__",    strlen("__SIZEOF_SIZE_T__"),    NULL, -1, 0, "8", 1);
    macro_define(&macros, "__SIZEOF_LONG_LONG__", strlen("__SIZEOF_LONG_LONG__"), NULL, -1, 0, "8", 1);

    for (int i = 0; i < n_defines; i++) {
        const char *def = defines[i];
        const char *eq  = strchr(def, '=');
        if (eq) {
            size_t nlen = (size_t)(eq - def);
            const char *val = eq + 1;
            macro_define(&macros, def, nlen, NULL, -1, 0, val, strlen(val));
        } else {
            macro_define(&macros, def, strlen(def), NULL, -1, 0, "1", 1);
        }
    }

    char *result = preprocess_internal(source,
                                       source_path ? source_path : ".",
                                       0, &macros, include_dirs, n_include_dirs);
    macro_table_free(&macros);
    return result;
}
