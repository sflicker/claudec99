#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
 * On compatible redefinition, incoming params are freed. */
static void macro_define(MacroTable *t, const char *name, size_t nlen,
                          char **params, int param_count,
                          const char *repl, size_t rlen) {
    MacroDef *existing = macro_find(t, name, nlen);
    if (existing) {
        /* free any incoming params regardless of outcome */
        if (params) {
            for (int i = 0; i < param_count; i++) free(params[i]);
            free(params);
        }
        if (strlen(existing->replacement) == rlen &&
            strncmp(existing->replacement, repl, rlen) == 0)
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
                                  int depth, MacroTable *macros);
static char *expand_macros_text(const char *text, MacroTable *macros);

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

/* ---- Substitute parameters in replacement text ----------------------- */

static char *substitute_args(const char *repl,
                              char **params, int param_count,
                              char **args, const size_t *arg_lens) {
    GrowBuf out;
    gbuf_init(&out, strlen(repl) * 2 + 1);
    const char *r = repl;
    while (*r) {
        if (isalpha((unsigned char)*r) || *r == '_') {
            const char *id_start = r;
            while (*r && (isalnum((unsigned char)*r) || *r == '_')) r++;
            size_t id_len = (size_t)(r - id_start);
            int found = 0;
            for (int i = 0; i < param_count; i++) {
                if (strlen(params[i]) == id_len &&
                    strncmp(params[i], id_start, id_len) == 0) {
                    gbuf_append(&out, args[i], arg_lens[i]);
                    found = 1;
                    break;
                }
            }
            if (!found) gbuf_append(&out, id_start, id_len);
        } else {
            gbuf_push(&out, *r++);
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
                    if (arg_count != def->param_count) {
                        fprintf(stderr,
                                "error: argument count mismatch for macro '%.*s':"
                                " expected %d, got %d\n",
                                (int)id_len, s + id_start,
                                def->param_count, arg_count);
                        for (int i = 0; i < arg_count; i++) free(args[i]);
                        free(args); free(arg_lens); free(out.data);
                        exit(1);
                    }
                    /* pre-expand each argument before substitution */
                    for (int i = 0; i < arg_count; i++) {
                        char *ea = expand_macros_text(args[i], macros);
                        free(args[i]);
                        args[i]     = ea;
                        arg_lens[i] = strlen(ea);
                    }
                    /* substitute args into replacement */
                    char *subst = substitute_args(def->replacement,
                                                   def->params, def->param_count,
                                                   args, arg_lens);
                    /* rescan the substituted text */
                    char *rescanned = expand_macros_text(subst, macros);
                    gbuf_append(&out, rescanned, strlen(rescanned));
                    free(rescanned); free(subst);
                    for (int i = 0; i < arg_count; i++) free(args[i]);
                    free(args); free(arg_lens);
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
        if (!isdigit((unsigned char)*repl)) {
            fprintf(stderr, "error: macro in #if does not expand to an integer\n");
            free(out_data); free(spliced_buf); exit(1);
        }
        long value = 0;
        while (isdigit((unsigned char)*repl))
            value = value * 10 + (*repl++ - '0');
        return value;
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

/* Unary expression: optional leading !, -, + (chained) then primary. */
static long eval_cond_unary(const char *s, size_t *in, MacroTable *macros,
                             char *out_data, char *spliced_buf) {
    char ops[32];
    int  nops = 0;

    while (s[*in] == '!' || s[*in] == '-' || s[*in] == '+') {
        ops[nops++] = s[(*in)++];
        while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
    }

    long value = eval_cond_primary(s, in, macros, out_data, spliced_buf);

    for (int i = nops - 1; i >= 0; i--) {
        if      (ops[i] == '!') value = (value == 0) ? 1L : 0L;
        else if (ops[i] == '-') value = -value;
    }

    return value;
}

/* Relational expression: unary (<, <=, >, >=) unary, left-associative. */
static long eval_cond_relational(const char *s, size_t *in, MacroTable *macros,
                                  char *out_data, char *spliced_buf) {
    long value = eval_cond_unary(s, in, macros, out_data, spliced_buf);

    for (;;) {
        while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
        if (s[*in] == '<' && s[*in + 1] == '=') {
            *in += 2;
            while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
            long rhs = eval_cond_unary(s, in, macros, out_data, spliced_buf);
            value = (value <= rhs) ? 1L : 0L;
        } else if (s[*in] == '>' && s[*in + 1] == '=') {
            *in += 2;
            while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
            long rhs = eval_cond_unary(s, in, macros, out_data, spliced_buf);
            value = (value >= rhs) ? 1L : 0L;
        } else if (s[*in] == '<' && s[*in + 1] != '<') {
            (*in)++;
            while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
            long rhs = eval_cond_unary(s, in, macros, out_data, spliced_buf);
            value = (value < rhs) ? 1L : 0L;
        } else if (s[*in] == '>' && s[*in + 1] != '>') {
            (*in)++;
            while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
            long rhs = eval_cond_unary(s, in, macros, out_data, spliced_buf);
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

/* Logical AND expression: equality { "&&" equality }, left-associative. */
static long eval_cond_logical_and(const char *s, size_t *in, MacroTable *macros,
                                   char *out_data, char *spliced_buf) {
    long value = eval_cond_equality(s, in, macros, out_data, spliced_buf);

    for (;;) {
        while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
        if (s[*in] == '&' && s[*in + 1] == '&') {
            *in += 2;
            while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
            long rhs = eval_cond_equality(s, in, macros, out_data, spliced_buf);
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

                macro_define(macros,
                             s + name_start, name_len,
                             params, param_count,
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
                            if (arg_count != def->param_count) {
                                fprintf(stderr,
                                        "error: argument count mismatch for macro '%.*s':"
                                        " expected %d, got %d\n",
                                        (int)id_len, s + id_start,
                                        def->param_count, arg_count);
                                for (int i = 0; i < arg_count; i++) free(args[i]);
                                free(args); free(arg_lens);
                                free(out.data); free(spliced);
                                exit(1);
                            }
                            /* pre-expand each argument */
                            for (int i = 0; i < arg_count; i++) {
                                char *ea = expand_macros_text(args[i], macros);
                                free(args[i]);
                                args[i]     = ea;
                                arg_lens[i] = strlen(ea);
                            }
                            /* substitute into replacement, then rescan */
                            char *subst     = substitute_args(def->replacement,
                                                               def->params, def->param_count,
                                                               args, arg_lens);
                            char *rescanned = expand_macros_text(subst, macros);
                            gbuf_append(&out, rescanned, strlen(rescanned));
                            free(rescanned); free(subst);
                            for (int i = 0; i < arg_count; i++) free(args[i]);
                            free(args); free(arg_lens);
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
    MacroTable macros;
    macro_table_init(&macros);
    char *result = preprocess_internal(source,
                                       source_path ? source_path : ".",
                                       0, &macros);
    macro_table_free(&macros);
    return result;
}
