#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum TokenKind {
    TK_NUM   = 0,
    TK_IDENT = 1,
    TK_PLUS  = 2,
    TK_MINUS = 3,
    TK_STAR  = 4,
    TK_SLASH = 5,
    TK_EOF   = 6
};

struct Token {
    enum TokenKind kind;
    int            val;
    char          *text;
};

static char *kind_name(enum TokenKind k) {
    switch (k) {
    case 0: return "NUM";
    case 1: return "IDENT";
    case 2: return "PLUS";
    case 3: return "MINUS";
    case 4: return "STAR";
    case 5: return "SLASH";
    case 6: return "EOF";
    default: return "UNKNOWN";
    }
}

static int is_digit(char c) { return c >= '0' && c <= '9'; }
static int is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}
static int is_alnum(char c) { return is_alpha(c) || is_digit(c); }

static int tokenize(const char *src, struct Token **out_tokens) {
    int cap = 8;
    int len = 0;
    struct Token *toks;
    const char *p;
    struct Token *t;
    const char *start;
    int id_len;
    char *buf;

    toks = malloc(cap * sizeof(struct Token));
    p = src;

    while (*p != '\0') {
        if (*p == ' ' || *p == '\t' || *p == '\n') {
            p++;
            continue;
        }

        if (len == cap) {
            cap = cap * 2;
            toks = realloc(toks, cap * sizeof(struct Token));
        }

        t = &toks[len];
        t->val  = 0;
        t->text = 0;

        if (is_digit(*p)) {
            t->kind = TK_NUM;
            while (is_digit(*p)) {
                t->val = t->val * 10 + (*p - '0');
                p++;
            }
            len++;
        } else if (is_alpha(*p)) {
            start = p;
            while (is_alnum(*p)) p++;
            id_len = p - start;
            buf = malloc(id_len + 1);
            memcpy(buf, start, id_len);
            buf[id_len] = '\0';
            t->kind = TK_IDENT;
            t->text = buf;
            len++;
        } else {
            if (*p == '+') {
                t->kind = TK_PLUS;  t->text = "+"; len++;
            } else if (*p == '-') {
                t->kind = TK_MINUS; t->text = "-"; len++;
            } else if (*p == '*') {
                t->kind = TK_STAR;  t->text = "*"; len++;
            } else if (*p == '/') {
                t->kind = TK_SLASH; t->text = "/"; len++;
            }
            p++;
        }
    }

    if (len == cap) {
        cap = cap + 1;
        toks = realloc(toks, cap * sizeof(struct Token));
    }
    toks[len].kind = TK_EOF;
    toks[len].val  = 0;
    toks[len].text = 0;
    len++;

    *out_tokens = toks;
    return len;
}

int main(void) {
    const char *src = "result + 42 * x - 7";
    struct Token *tokens;
    int n;
    int i;
    struct Token *t;

    n = tokenize(src, &tokens);

    for (i = 0; i < n; i++) {
        t = &tokens[i];
        if (t->kind == TK_NUM) {
            printf("%s %d\n", kind_name(t->kind), t->val);
        } else if (t->kind == TK_IDENT) {
            printf("%s %s\n", kind_name(t->kind), t->text);
            free(t->text);
        } else {
            printf("%s\n", kind_name(t->kind));
        }
    }

    free(tokens);
    return 0;
}
