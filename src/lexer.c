#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "strbuf.h"
#include "util.h"

/* Look up or create a SourceFile for the given path in the lexer's pool.
 * Returns a pointer into the pool (valid for the lifetime of the lexer). */
static SourceFile *lexer_intern_file(Lexer *lexer, const char *path) {
    for (int i = 0; i < lexer->n_files; i++) {
        if (strcmp(lexer->file_pool[i]->path, path) == 0)
            return lexer->file_pool[i];
    }
    if (lexer->n_files == lexer->files_cap) {
        lexer->files_cap = lexer->files_cap * 2 + 4;
        lexer->file_pool = realloc(lexer->file_pool,
                                   (size_t)lexer->files_cap * sizeof(SourceFile *));
        if (!lexer->file_pool) {
            fprintf(stderr, "error: out of memory in lexer\n");
            exit(1);
        }
    }
    SourceFile *sf = malloc(sizeof(SourceFile));
    if (!sf) { fprintf(stderr, "error: out of memory in lexer\n"); exit(1); }
    sf->path = util_strdup(path);
    if (!sf->path) { fprintf(stderr, "error: out of memory in lexer\n"); exit(1); }
    lexer->file_pool[lexer->n_files++] = sf;
    return sf;
}

/* Stage 95-08: allocate a private copy of `len` bytes from `data` in the
 * lexer's string pool.  A null sentinel is appended at data[len] so that
 * callers can safely pass the result to string.h functions.  The returned
 * pointer is stable for the lifetime of the lexer because each string is
 * an independent malloc, not part of a relocatable buffer. */
const char *lexer_store_bytes(Lexer *lexer, const char *data, size_t len) {
    char *copy = (char *)malloc(len + 1);
    if (!copy) { fprintf(stderr, "error: out of memory in lexer\n"); exit(1); }
    memcpy(copy, data, len);
    copy[len] = '\0';
    vec_push(&lexer->str_pool, &copy);
    return copy;
}

void lexer_init(Lexer *lexer, const char *source, const char *filename) {
    lexer->source    = source;
    lexer->pos       = 0;
    lexer->line      = 1;
    lexer->col       = 1;
    lexer->file_pool = NULL;
    lexer->n_files   = 0;
    lexer->files_cap = 0;
    lexer->file      = filename ? lexer_intern_file(lexer, filename) : NULL;
    vec_init(&lexer->str_pool, sizeof(char *));
}

void lexer_free(Lexer *lexer) {
    for (int i = 0; i < lexer->n_files; i++) {
        free(lexer->file_pool[i]->path);
        free(lexer->file_pool[i]);
    }
    free(lexer->file_pool);
    lexer->file_pool = NULL;
    lexer->n_files   = 0;
    lexer->files_cap = 0;
    /* Free all lexer-owned strings. */
    {
        size_t i;
        for (i = 0; i < lexer->str_pool.len; i++) {
            char **pp = (char **)vec_get(&lexer->str_pool, i);
            free(*pp);
        }
    }
    vec_free(&lexer->str_pool);
}

/* Advance past one character, updating line/col tracking. */
static void lexer_advance(Lexer *lexer) {
    if (lexer->source[lexer->pos] == '\n') {
        lexer->line++;
        lexer->col = 1;
    } else {
        lexer->col++;
    }
    lexer->pos++;
}

/* Parse and consume a \x01<line>:<path>\n location marker emitted by the
 * preprocessor at #include boundaries.  Called when source[pos] == '\x01'. */
static void lexer_parse_location_marker(Lexer *lexer) {
    lexer->pos++; /* skip \x01 */
    /* Parse decimal line number. */
    int new_line = 0;
    while (lexer->source[lexer->pos] >= '0' && lexer->source[lexer->pos] <= '9') {
        new_line = new_line * 10 + (lexer->source[lexer->pos] - '0');
        lexer->pos++;
    }
    if (lexer->source[lexer->pos] == ':')
        lexer->pos++; /* skip ':' */
    /* Path runs to end of line. */
    size_t path_start = (size_t)lexer->pos;
    while (lexer->source[lexer->pos] && lexer->source[lexer->pos] != '\n')
        lexer->pos++;
    size_t path_len = (size_t)lexer->pos - path_start;
    if (lexer->source[lexer->pos] == '\n')
        lexer->pos++; /* skip '\n' */
    /* Update lexer state. */
    lexer->line = new_line;
    lexer->col  = 1;
    if (path_len > 0) {
        char path_buf[512];
        if (path_len >= sizeof(path_buf)) path_len = sizeof(path_buf) - 1;
        memcpy(path_buf, lexer->source + path_start, path_len);
        path_buf[path_len] = '\0';
        lexer->file = lexer_intern_file(lexer, path_buf);
    }
}

static void lexer_skip_whitespace(Lexer *lexer) {
    for (;;) {
        char c = lexer->source[lexer->pos];
        if (c == '\x01') {
            lexer_parse_location_marker(lexer);
        } else if (c && isspace((unsigned char)c)) {
            lexer_advance(lexer);
        } else {
            break;
        }
    }
}

/* Record the current lexer position into a token before consuming chars. */
static void token_set_pos(Token *token, const Lexer *lexer) {
    token->line = lexer->line;
    token->col  = lexer->col;
    token->file = lexer->file;
}

Token lexer_next_token(Lexer *lexer) {
    Token token;
    memset(&token, 0, sizeof(token));
    token.value = ""; /* safe default so %s printing of TOKEN_EOF works */

    lexer_skip_whitespace(lexer);

    /* Record position at the first character of this token. */
    token_set_pos(&token, lexer);

    char c = lexer->source[lexer->pos];
    if (c == '\0') {
        token.type = TOKEN_EOF;
        return token;
    }

    /* Single-character punctuator tokens. */
    if (c == '(') { token.type = TOKEN_LPAREN;    token.value = "(";  token.value_len = 1; token.lexeme = token.value; token.lexeme_len = 1; lexer_advance(lexer); return token; }
    if (c == ')') { token.type = TOKEN_RPAREN;    token.value = ")";  token.value_len = 1; token.lexeme = token.value; token.lexeme_len = 1; lexer_advance(lexer); return token; }
    if (c == '{') { token.type = TOKEN_LBRACE;    token.value = "{";  token.value_len = 1; token.lexeme = token.value; token.lexeme_len = 1; lexer_advance(lexer); return token; }
    if (c == '}') { token.type = TOKEN_RBRACE;    token.value = "}";  token.value_len = 1; token.lexeme = token.value; token.lexeme_len = 1; lexer_advance(lexer); return token; }
    if (c == '[') { token.type = TOKEN_LBRACKET;  token.value = "[";  token.value_len = 1; token.lexeme = token.value; token.lexeme_len = 1; lexer_advance(lexer); return token; }
    if (c == ']') { token.type = TOKEN_RBRACKET;  token.value = "]";  token.value_len = 1; token.lexeme = token.value; token.lexeme_len = 1; lexer_advance(lexer); return token; }
    if (c == ';') { token.type = TOKEN_SEMICOLON; token.value = ";";  token.value_len = 1; token.lexeme = token.value; token.lexeme_len = 1; lexer_advance(lexer); return token; }
    if (c == ':') { token.type = TOKEN_COLON;     token.value = ":";  token.value_len = 1; token.lexeme = token.value; token.lexeme_len = 1; lexer_advance(lexer); return token; }
    if (c == '?') { token.type = TOKEN_QUESTION;  token.value = "?";  token.value_len = 1; token.lexeme = token.value; token.lexeme_len = 1; lexer_advance(lexer); return token; }
    if (c == ',') { token.type = TOKEN_COMMA;     token.value = ",";  token.value_len = 1; token.lexeme = token.value; token.lexeme_len = 1; lexer_advance(lexer); return token; }
    if (c == '.') {
        if (lexer->source[lexer->pos + 1] == '.' && lexer->source[lexer->pos + 2] == '.') {
            token.type = TOKEN_ELLIPSIS; token.value = "..."; token.value_len = 3;
            token.lexeme = token.value; token.lexeme_len = 3;
            lexer_advance(lexer); lexer_advance(lexer); lexer_advance(lexer);
            return token;
        }
        token.type = TOKEN_DOT; token.value = "."; token.value_len = 1;
        token.lexeme = token.value; token.lexeme_len = 1;
        lexer_advance(lexer); return token;
    }
    if (c == '+') {
        if (lexer->source[lexer->pos + 1] == '+') { token.type = TOKEN_INCREMENT;   token.value = "++"; token.value_len = 2; token.lexeme = token.value; token.lexeme_len = 2; lexer_advance(lexer); lexer_advance(lexer); return token; }
        if (lexer->source[lexer->pos + 1] == '=') { token.type = TOKEN_PLUS_ASSIGN; token.value = "+="; token.value_len = 2; token.lexeme = token.value; token.lexeme_len = 2; lexer_advance(lexer); lexer_advance(lexer); return token; }
        token.type = TOKEN_PLUS; token.value = "+"; token.value_len = 1; token.lexeme = token.value; token.lexeme_len = 1; lexer_advance(lexer); return token;
    }
    if (c == '-') {
        if (lexer->source[lexer->pos + 1] == '-') { token.type = TOKEN_DECREMENT;    token.value = "--"; token.value_len = 2; token.lexeme = token.value; token.lexeme_len = 2; lexer_advance(lexer); lexer_advance(lexer); return token; }
        if (lexer->source[lexer->pos + 1] == '=') { token.type = TOKEN_MINUS_ASSIGN; token.value = "-="; token.value_len = 2; token.lexeme = token.value; token.lexeme_len = 2; lexer_advance(lexer); lexer_advance(lexer); return token; }
        if (lexer->source[lexer->pos + 1] == '>') { token.type = TOKEN_ARROW;        token.value = "->"; token.value_len = 2; token.lexeme = token.value; token.lexeme_len = 2; lexer_advance(lexer); lexer_advance(lexer); return token; }
        token.type = TOKEN_MINUS; token.value = "-"; token.value_len = 1; token.lexeme = token.value; token.lexeme_len = 1; lexer_advance(lexer); return token;
    }
    if (c == '*') {
        if (lexer->source[lexer->pos + 1] == '=') { token.type = TOKEN_STAR_ASSIGN;  token.value = "*="; token.value_len = 2; token.lexeme = token.value; token.lexeme_len = 2; lexer_advance(lexer); lexer_advance(lexer); return token; }
        token.type = TOKEN_STAR;    token.value = "*"; token.value_len = 1; token.lexeme = token.value; token.lexeme_len = 1; lexer_advance(lexer); return token;
    }
    if (c == '/') {
        if (lexer->source[lexer->pos + 1] == '=') { token.type = TOKEN_SLASH_ASSIGN;  token.value = "/="; token.value_len = 2; token.lexeme = token.value; token.lexeme_len = 2; lexer_advance(lexer); lexer_advance(lexer); return token; }
        token.type = TOKEN_SLASH;   token.value = "/"; token.value_len = 1; token.lexeme = token.value; token.lexeme_len = 1; lexer_advance(lexer); return token;
    }
    if (c == '%') {
        if (lexer->source[lexer->pos + 1] == '=') { token.type = TOKEN_PERCENT_ASSIGN; token.value = "%="; token.value_len = 2; token.lexeme = token.value; token.lexeme_len = 2; lexer_advance(lexer); lexer_advance(lexer); return token; }
        token.type = TOKEN_PERCENT; token.value = "%"; token.value_len = 1; token.lexeme = token.value; token.lexeme_len = 1; lexer_advance(lexer); return token;
    }
    if (c == '~') { token.type = TOKEN_TILDE; token.value = "~"; token.value_len = 1; token.lexeme = token.value; token.lexeme_len = 1; lexer_advance(lexer); return token; }

    /* Two-character or single-character relational/equality tokens */
    {
        char n = lexer->source[lexer->pos + 1];
        if (c == '=' && n == '=') { token.type = TOKEN_EQ; token.value = "=="; token.value_len = 2; token.lexeme = token.value; token.lexeme_len = 2; lexer_advance(lexer); lexer_advance(lexer); return token; }
        if (c == '=') { token.type = TOKEN_ASSIGN; token.value = "="; token.value_len = 1; token.lexeme = token.value; token.lexeme_len = 1; lexer_advance(lexer); return token; }
        if (c == '!' && n == '=') { token.type = TOKEN_NE; token.value = "!="; token.value_len = 2; token.lexeme = token.value; token.lexeme_len = 2; lexer_advance(lexer); lexer_advance(lexer); return token; }
        if (c == '!') { token.type = TOKEN_BANG; token.value = "!"; token.value_len = 1; token.lexeme = token.value; token.lexeme_len = 1; lexer_advance(lexer); return token; }
        if (c == '<' && n == '<') {
            if (lexer->source[lexer->pos + 2] == '=') { token.type = TOKEN_LEFT_SHIFT_ASSIGN;  token.value = "<<="; token.value_len = 3; token.lexeme = token.value; token.lexeme_len = 3; lexer_advance(lexer); lexer_advance(lexer); lexer_advance(lexer); return token; }
            token.type = TOKEN_LEFT_SHIFT;  token.value = "<<"; token.value_len = 2; token.lexeme = token.value; token.lexeme_len = 2; lexer_advance(lexer); lexer_advance(lexer); return token;
        }
        if (c == '>' && n == '>') {
            if (lexer->source[lexer->pos + 2] == '=') { token.type = TOKEN_RIGHT_SHIFT_ASSIGN; token.value = ">>="; token.value_len = 3; token.lexeme = token.value; token.lexeme_len = 3; lexer_advance(lexer); lexer_advance(lexer); lexer_advance(lexer); return token; }
            token.type = TOKEN_RIGHT_SHIFT; token.value = ">>"; token.value_len = 2; token.lexeme = token.value; token.lexeme_len = 2; lexer_advance(lexer); lexer_advance(lexer); return token;
        }
        if (c == '<' && n == '=') { token.type = TOKEN_LE; token.value = "<="; token.value_len = 2; token.lexeme = token.value; token.lexeme_len = 2; lexer_advance(lexer); lexer_advance(lexer); return token; }
        if (c == '>' && n == '=') { token.type = TOKEN_GE; token.value = ">="; token.value_len = 2; token.lexeme = token.value; token.lexeme_len = 2; lexer_advance(lexer); lexer_advance(lexer); return token; }
        if (c == '<') { token.type = TOKEN_LT; token.value = "<"; token.value_len = 1; token.lexeme = token.value; token.lexeme_len = 1; lexer_advance(lexer); return token; }
        if (c == '>') { token.type = TOKEN_GT; token.value = ">"; token.value_len = 1; token.lexeme = token.value; token.lexeme_len = 1; lexer_advance(lexer); return token; }
        if (c == '&' && n == '&') { token.type = TOKEN_AND_AND;    token.value = "&&"; token.value_len = 2; token.lexeme = token.value; token.lexeme_len = 2; lexer_advance(lexer); lexer_advance(lexer); return token; }
        if (c == '&' && n == '=') { token.type = TOKEN_AMP_ASSIGN; token.value = "&="; token.value_len = 2; token.lexeme = token.value; token.lexeme_len = 2; lexer_advance(lexer); lexer_advance(lexer); return token; }
        if (c == '&') { token.type = TOKEN_AMPERSAND; token.value = "&"; token.value_len = 1; token.lexeme = token.value; token.lexeme_len = 1; lexer_advance(lexer); return token; }
        if (c == '|' && n == '|') { token.type = TOKEN_OR_OR;      token.value = "||"; token.value_len = 2; token.lexeme = token.value; token.lexeme_len = 2; lexer_advance(lexer); lexer_advance(lexer); return token; }
        if (c == '|' && n == '=') { token.type = TOKEN_PIPE_ASSIGN; token.value = "|="; token.value_len = 2; token.lexeme = token.value; token.lexeme_len = 2; lexer_advance(lexer); lexer_advance(lexer); return token; }
        if (c == '|') { token.type = TOKEN_PIPE;  token.value = "|"; token.value_len = 1; token.lexeme = token.value; token.lexeme_len = 1; lexer_advance(lexer); return token; }
        if (c == '^' && n == '=') { token.type = TOKEN_CARET_ASSIGN; token.value = "^="; token.value_len = 2; token.lexeme = token.value; token.lexeme_len = 2; lexer_advance(lexer); lexer_advance(lexer); return token; }
        if (c == '^') { token.type = TOKEN_CARET; token.value = "^"; token.value_len = 1; token.lexeme = token.value; token.lexeme_len = 1; lexer_advance(lexer); return token; }
    }

    /* String literals: double-quoted, with supported backslash escape
     * sequences decoded into their byte values.  The StrBuf accumulates
     * the decoded bytes; the final result is copied into a lexer-owned
     * heap string via lexer_store_bytes, which also appends a null
     * sentinel (not counted in value_len).
     *
     * Three rejection cases:
     *   - newline byte before the closing quote
     *   - end-of-file before the closing quote
     *   - any backslash escape outside the supported set
     *
     * Embedded NUL bytes (from \0) are preserved; callers must use
     * value_len rather than strlen(value) to iterate. */
    if (c == '"') {
        StrBuf decoded;
        strbuf_init(&decoded);
        lexer_advance(lexer); /* skip opening '"' */
        for (;;) {
            char ch = lexer->source[lexer->pos];
            if (ch == '"') {
                lexer_advance(lexer);
                break;
            }
            if (ch == '\0') {
                strbuf_free(&decoded);
                fprintf(stderr, "error: unterminated string literal\n");
                exit(1);
            }
            if (ch == '\n') {
                strbuf_free(&decoded);
                fprintf(stderr, "error: newline in string literal\n");
                exit(1);
            }
            if (ch == '\\') {
                char next = lexer->source[lexer->pos + 1];
                char dc;
                if (next == 'x') {
                    lexer_advance(lexer); /* skip \ */
                    lexer_advance(lexer); /* skip x */
                    if (!isxdigit((unsigned char)lexer->source[lexer->pos])) {
                        strbuf_free(&decoded);
                        fprintf(stderr, "error: invalid hex escape in string literal\n");
                        exit(1);
                    }
                    unsigned int val = 0;
                    while (isxdigit((unsigned char)lexer->source[lexer->pos])) {
                        char hc = lexer->source[lexer->pos];
                        unsigned int digit;
                        if (hc >= '0' && hc <= '9') digit = (unsigned int)(hc - '0');
                        else if (hc >= 'a' && hc <= 'f') digit = (unsigned int)(hc - 'a') + 10u;
                        else digit = (unsigned int)(hc - 'A') + 10u;
                        val = val * 16u + digit;
                        lexer_advance(lexer);
                    }
                    dc = (char)(val & 0xFFu);
                } else if (next >= '0' && next <= '7') {
                    lexer_advance(lexer); /* skip \ */
                    unsigned int val = 0;
                    int count = 0;
                    while (count < 3 && lexer->source[lexer->pos] >= '0' &&
                           lexer->source[lexer->pos] <= '7') {
                        val = val * 8u + (unsigned int)(lexer->source[lexer->pos] - '0');
                        lexer_advance(lexer);
                        count++;
                    }
                    dc = (char)(val & 0xFFu);
                } else {
                    switch (next) {
                    case 'n':  dc = '\n'; break;
                    case 't':  dc = '\t'; break;
                    case 'r':  dc = '\r'; break;
                    case '\\': dc = '\\'; break;
                    case '"':  dc = '"';  break;
                    default:
                        strbuf_free(&decoded);
                        fprintf(stderr,
                                "error: invalid escape sequence in string literal\n");
                        exit(1);
                    }
                    lexer_advance(lexer); lexer_advance(lexer); /* skip \ + escape char */
                }
                strbuf_append_char(&decoded, dc);
                continue;
            }
            strbuf_append_char(&decoded, ch);
            lexer_advance(lexer);
        }
        token.type = TOKEN_STRING_LITERAL;
        token.value     = lexer_store_bytes(lexer, decoded.data, decoded.len);
        token.value_len = decoded.len;
        token.lexeme    = token.value;
        token.lexeme_len = token.value_len;
        strbuf_free(&decoded);
        return token;
    }

    /* Character literals: single-quoted single character or one of the
     * supported backslash escapes.  The decoded byte is stored via
     * lexer_store_bytes (1 byte + null sentinel); the integer value is
     * also in long_value with literal_type = TYPE_INT.
     *
     * Rejected forms: empty '', EOF/newline before close, multi-char
     * constants, and any unsupported backslash escape. */
    if (c == '\'') {
        lexer_advance(lexer); /* skip opening '\'' */
        char ch = lexer->source[lexer->pos];
        if (ch == '\0') {
            fprintf(stderr, "error: unterminated character literal\n");
            exit(1);
        }
        if (ch == '\n') {
            fprintf(stderr, "error: newline in character literal\n");
            exit(1);
        }
        if (ch == '\'') {
            fprintf(stderr, "error: empty character literal\n");
            exit(1);
        }
        char dc;
        if (ch == '\\') {
            char next = lexer->source[lexer->pos + 1];
            if (next == 'x') {
                lexer_advance(lexer); /* skip \ */
                lexer_advance(lexer); /* skip x */
                if (!isxdigit((unsigned char)lexer->source[lexer->pos])) {
                    fprintf(stderr,
                            "error: invalid hex escape in character literal\n");
                    exit(1);
                }
                unsigned int val = 0;
                while (isxdigit((unsigned char)lexer->source[lexer->pos])) {
                    char hc = lexer->source[lexer->pos];
                    unsigned int digit;
                    if (hc >= '0' && hc <= '9') digit = (unsigned int)(hc - '0');
                    else if (hc >= 'a' && hc <= 'f') digit = (unsigned int)(hc - 'a') + 10u;
                    else digit = (unsigned int)(hc - 'A') + 10u;
                    val = val * 16u + digit;
                    lexer_advance(lexer);
                }
                dc = (char)(val & 0xFFu);
            } else if (next >= '0' && next <= '7') {
                lexer_advance(lexer); /* skip \ */
                unsigned int val = 0;
                int count = 0;
                while (count < 3 && lexer->source[lexer->pos] >= '0' &&
                       lexer->source[lexer->pos] <= '7') {
                    val = val * 8u + (unsigned int)(lexer->source[lexer->pos] - '0');
                    lexer_advance(lexer);
                    count++;
                }
                dc = (char)(val & 0xFFu);
            } else {
                switch (next) {
                case 'a':  dc = 7;    break;
                case 'b':  dc = 8;    break;
                case 'f':  dc = 12;   break;
                case 'n':  dc = 10;   break;
                case 'r':  dc = 13;   break;
                case 't':  dc = 9;    break;
                case 'v':  dc = 11;   break;
                case '\\': dc = '\\'; break;
                case '\'': dc = '\''; break;
                case '"':  dc = '"';  break;
                case '?':  dc = '?';  break;
                default:
                    fprintf(stderr,
                            "error: invalid escape sequence in character literal\n");
                    exit(1);
                }
                lexer_advance(lexer); lexer_advance(lexer); /* skip \ + escape char */
            }
        } else {
            dc = ch;
            lexer_advance(lexer);
        }
        if (lexer->source[lexer->pos] != '\'') {
            int p = lexer->pos;
            while (lexer->source[p] != '\0' &&
                   lexer->source[p] != '\n' &&
                   lexer->source[p] != '\'') {
                p++;
            }
            if (lexer->source[p] == '\'') {
                fprintf(stderr, "error: multi character constant\n");
            } else {
                fprintf(stderr, "error: unterminated character literal\n");
            }
            exit(1);
        }
        lexer_advance(lexer); /* skip closing '\'' */
        token.value      = lexer_store_bytes(lexer, &dc, 1);
        token.value_len  = 1;
        token.lexeme     = token.value;
        token.lexeme_len = 1;
        token.long_value   = (long)(unsigned char)dc;
        token.literal_type = TYPE_INT;
        token.type = TOKEN_CHAR_LITERAL;
        return token;
    }

    /* Wide-character literals (L'A', U'A', u'A') are out of scope.
     * Reject explicitly here so the prefix is not consumed by the
     * identifier branch below. */
    if ((c == 'L' || c == 'U' || c == 'u') &&
        lexer->source[lexer->pos + 1] == '\'') {
        fprintf(stderr, "error: wide character literal not supported\n");
        exit(1);
    }

    /* Integer literals: decimal or hexadecimal (0x/0X prefix).
     * Optional U/u and L/l suffixes set unsigned and size.
     * Without a suffix, type is int when value fits signed 32-bit, else long. */
    if (isdigit(c)) {
        char num_buf[64];
        int i = 0;
        int is_hex = 0;

        /* Detect hex prefix 0x / 0X */
        if (c == '0' && (lexer->source[lexer->pos + 1] == 'x' ||
                         lexer->source[lexer->pos + 1] == 'X')) {
            is_hex = 1;
            num_buf[i++] = '0';
            lexer_advance(lexer);
            num_buf[i++] = lexer->source[lexer->pos]; /* 'x' or 'X' */
            lexer_advance(lexer);
            if (!isxdigit((unsigned char)lexer->source[lexer->pos])) {
                fprintf(stderr, "error: invalid hexadecimal integer literal\n");
                exit(1);
            }
            while (isxdigit((unsigned char)lexer->source[lexer->pos]) && i < 62) {
                num_buf[i++] = lexer->source[lexer->pos];
                lexer_advance(lexer);
            }
        } else {
            while (isdigit(lexer->source[lexer->pos]) && i < 62) {
                num_buf[i++] = lexer->source[lexer->pos];
                lexer_advance(lexer);
            }
        }
        num_buf[i] = '\0';

        int l_count = 0;
        int has_unsigned_suffix = 0;
        while (lexer->source[lexer->pos] == 'U' || lexer->source[lexer->pos] == 'u' ||
               lexer->source[lexer->pos] == 'L' || lexer->source[lexer->pos] == 'l') {
            char sc = lexer->source[lexer->pos];
            lexer_advance(lexer);
            if (sc == 'U' || sc == 'u') has_unsigned_suffix = 1;
            else l_count++;
        }
        if (l_count > 2) {
            fprintf(stderr, "error: integer literal has too many 'L' suffixes\n");
            exit(1);
        }
        int has_ll_suffix   = (l_count == 2);
        int has_long_suffix = (l_count == 1);
        errno = 0;
        char *end = NULL;
        unsigned long parsed = strtoul(num_buf, &end, is_hex ? 16 : 10);
        int too_large = (errno == ERANGE) ||
                        (!has_unsigned_suffix && !has_ll_suffix && parsed > (unsigned long)LONG_MAX);
        if (too_large) {
            fprintf(stderr,
                    "error: integer literal '%s' too large for supported integer types\n",
                    num_buf);
            exit(1);
        }
        token.long_value = (long)parsed;
        token.is_unsigned = has_unsigned_suffix;
        if (has_ll_suffix) {
            token.literal_type = has_unsigned_suffix ? TYPE_UNSIGNED_LONG_LONG : TYPE_LONG_LONG;
        } else if (has_long_suffix) {
            token.literal_type = TYPE_LONG;
        } else if (has_unsigned_suffix) {
            token.literal_type = (parsed <= 0xFFFFFFFFUL) ? TYPE_INT : TYPE_LONG;
        } else if (parsed <= (unsigned long)INT_MAX) {
            token.literal_type = TYPE_INT;
        } else {
            token.literal_type = TYPE_LONG;
        }
        token.value      = lexer_store_bytes(lexer, num_buf, (size_t)i);
        token.value_len  = (size_t)i;
        token.lexeme     = token.value;
        token.lexeme_len = token.value_len;
        token.type = TOKEN_INT_LITERAL;
        return token;
    }

    /* Keywords and identifiers.  Build the identifier in a local buffer,
     * match against the keyword table, then either assign the matching
     * static string literal (for keywords) or intern a heap copy (for
     * user identifiers). */
    if (isalpha(c) || c == '_') {
        StrBuf ident_buf;
        strbuf_init(&ident_buf);
        while (isalnum(lexer->source[lexer->pos]) || lexer->source[lexer->pos] == '_') {
            strbuf_append_char(&ident_buf, lexer->source[lexer->pos]);
            lexer_advance(lexer);
        }
        strbuf_null_terminate(&ident_buf);

        if      (strcmp(ident_buf.data, "int")      == 0) { token.type = TOKEN_INT;      token.value = "int";      token.value_len = 3; }
        else if (strcmp(ident_buf.data, "char")     == 0) { token.type = TOKEN_CHAR;     token.value = "char";     token.value_len = 4; }
        else if (strcmp(ident_buf.data, "short")    == 0) { token.type = TOKEN_SHORT;    token.value = "short";    token.value_len = 5; }
        else if (strcmp(ident_buf.data, "long")     == 0) { token.type = TOKEN_LONG;     token.value = "long";     token.value_len = 4; }
        else if (strcmp(ident_buf.data, "return")   == 0) { token.type = TOKEN_RETURN;   token.value = "return";   token.value_len = 6; }
        else if (strcmp(ident_buf.data, "if")       == 0) { token.type = TOKEN_IF;       token.value = "if";       token.value_len = 2; }
        else if (strcmp(ident_buf.data, "else")     == 0) { token.type = TOKEN_ELSE;     token.value = "else";     token.value_len = 4; }
        else if (strcmp(ident_buf.data, "while")    == 0) { token.type = TOKEN_WHILE;    token.value = "while";    token.value_len = 5; }
        else if (strcmp(ident_buf.data, "do")       == 0) { token.type = TOKEN_DO;       token.value = "do";       token.value_len = 2; }
        else if (strcmp(ident_buf.data, "for")      == 0) { token.type = TOKEN_FOR;      token.value = "for";      token.value_len = 3; }
        else if (strcmp(ident_buf.data, "break")    == 0) { token.type = TOKEN_BREAK;    token.value = "break";    token.value_len = 5; }
        else if (strcmp(ident_buf.data, "continue") == 0) { token.type = TOKEN_CONTINUE; token.value = "continue"; token.value_len = 8; }
        else if (strcmp(ident_buf.data, "switch")   == 0) { token.type = TOKEN_SWITCH;   token.value = "switch";   token.value_len = 6; }
        else if (strcmp(ident_buf.data, "default")  == 0) { token.type = TOKEN_DEFAULT;  token.value = "default";  token.value_len = 7; }
        else if (strcmp(ident_buf.data, "case")     == 0) { token.type = TOKEN_CASE;     token.value = "case";     token.value_len = 4; }
        else if (strcmp(ident_buf.data, "goto")     == 0) { token.type = TOKEN_GOTO;     token.value = "goto";     token.value_len = 4; }
        else if (strcmp(ident_buf.data, "sizeof")   == 0) { token.type = TOKEN_SIZEOF;   token.value = "sizeof";   token.value_len = 6; }
        else if (strcmp(ident_buf.data, "extern")   == 0) { token.type = TOKEN_EXTERN;   token.value = "extern";   token.value_len = 6; }
        else if (strcmp(ident_buf.data, "static")   == 0) { token.type = TOKEN_STATIC;   token.value = "static";   token.value_len = 6; }
        else if (strcmp(ident_buf.data, "typedef")  == 0) { token.type = TOKEN_TYPEDEF;  token.value = "typedef";  token.value_len = 7; }
        else if (strcmp(ident_buf.data, "enum")     == 0) { token.type = TOKEN_ENUM;     token.value = "enum";     token.value_len = 4; }
        else if (strcmp(ident_buf.data, "struct")   == 0) { token.type = TOKEN_STRUCT;   token.value = "struct";   token.value_len = 6; }
        else if (strcmp(ident_buf.data, "union")    == 0) { token.type = TOKEN_UNION;    token.value = "union";    token.value_len = 5; }
        else if (strcmp(ident_buf.data, "void")     == 0) { token.type = TOKEN_VOID;     token.value = "void";     token.value_len = 4; }
        else if (strcmp(ident_buf.data, "const")    == 0) { token.type = TOKEN_CONST;    token.value = "const";    token.value_len = 5; }
        else if (strcmp(ident_buf.data, "volatile") == 0) { token.type = TOKEN_VOLATILE; token.value = "volatile"; token.value_len = 8; }
        else if (strcmp(ident_buf.data, "signed")   == 0) { token.type = TOKEN_SIGNED;   token.value = "signed";   token.value_len = 6; }
        else if (strcmp(ident_buf.data, "unsigned") == 0) { token.type = TOKEN_UNSIGNED; token.value = "unsigned"; token.value_len = 8; }
        else if (strcmp(ident_buf.data, "_Bool")    == 0) { token.type = TOKEN_BOOL;     token.value = "_Bool";    token.value_len = 5; }
        else {
            token.type = TOKEN_IDENTIFIER;
            token.value     = lexer_store_bytes(lexer, ident_buf.data, ident_buf.len);
            token.value_len = ident_buf.len;
        }
        strbuf_free(&ident_buf);
        token.lexeme     = token.value;
        token.lexeme_len = token.value_len;
        return token;
    }

    /* Unknown token: store the single character in the lexer's string pool. */
    {
        char unk[1];
        unk[0] = c;
        token.type      = TOKEN_UNKNOWN;
        token.value     = lexer_store_bytes(lexer, unk, 1);
        token.value_len = 1;
        token.lexeme    = token.value;
        token.lexeme_len = 1;
        lexer_advance(lexer);
        return token;
    }
}

const char *token_display_name(TokenType type) {
    switch (type) {
        case TOKEN_EOF:              return "<EOF>";
        case TOKEN_INT:              return "'int'";
        case TOKEN_CHAR:             return "'char'";
        case TOKEN_SHORT:            return "'short'";
        case TOKEN_LONG:             return "'long'";
        case TOKEN_RETURN:           return "'return'";
        case TOKEN_IF:               return "'if'";
        case TOKEN_ELSE:             return "'else'";
        case TOKEN_WHILE:            return "'while'";
        case TOKEN_DO:               return "'do'";
        case TOKEN_FOR:              return "'for'";
        case TOKEN_BREAK:            return "'break'";
        case TOKEN_CONTINUE:         return "'continue'";
        case TOKEN_SWITCH:           return "'switch'";
        case TOKEN_DEFAULT:          return "'default'";
        case TOKEN_CASE:             return "'case'";
        case TOKEN_GOTO:             return "'goto'";
        case TOKEN_SIZEOF:           return "'sizeof'";
        case TOKEN_EXTERN:           return "'extern'";
        case TOKEN_STATIC:           return "'static'";
        case TOKEN_TYPEDEF:          return "'typedef'";
        case TOKEN_ENUM:             return "'enum'";
        case TOKEN_STRUCT:           return "'struct'";
        case TOKEN_UNION:            return "'union'";
        case TOKEN_VOID:             return "'void'";
        case TOKEN_BOOL:             return "'_Bool'";
        case TOKEN_CONST:            return "'const'";
        case TOKEN_VOLATILE:         return "'volatile'";
        case TOKEN_SIGNED:           return "'signed'";
        case TOKEN_UNSIGNED:         return "'unsigned'";
        case TOKEN_IDENTIFIER:       return "identifier";
        case TOKEN_INT_LITERAL:      return "integer literal";
        case TOKEN_STRING_LITERAL:   return "string literal";
        case TOKEN_CHAR_LITERAL:     return "character literal";
        case TOKEN_LPAREN:           return "'('";
        case TOKEN_RPAREN:           return "')'";
        case TOKEN_LBRACE:           return "'{'";
        case TOKEN_RBRACE:           return "'}'";
        case TOKEN_LBRACKET:         return "'['";
        case TOKEN_RBRACKET:         return "']'";
        case TOKEN_SEMICOLON:        return "';'";
        case TOKEN_COLON:            return "':'";
        case TOKEN_COMMA:            return "','";
        case TOKEN_PLUS:             return "'+'";
        case TOKEN_MINUS:            return "'-'";
        case TOKEN_STAR:             return "'*'";
        case TOKEN_SLASH:            return "'/'";
        case TOKEN_PERCENT:          return "'%'";
        case TOKEN_BANG:             return "'!'";
        case TOKEN_TILDE:            return "'~'";
        case TOKEN_ASSIGN:           return "'='";
        case TOKEN_PLUS_ASSIGN:      return "'+='";
        case TOKEN_MINUS_ASSIGN:     return "'-='";
        case TOKEN_STAR_ASSIGN:      return "'*='";
        case TOKEN_SLASH_ASSIGN:     return "'/='";
        case TOKEN_PERCENT_ASSIGN:   return "'%='";
        case TOKEN_LEFT_SHIFT_ASSIGN:  return "'<<='";
        case TOKEN_RIGHT_SHIFT_ASSIGN: return "'>>='";
        case TOKEN_AMP_ASSIGN:       return "'&='";
        case TOKEN_CARET_ASSIGN:     return "'^='";
        case TOKEN_PIPE_ASSIGN:      return "'|='";
        case TOKEN_INCREMENT:        return "'++'";
        case TOKEN_DECREMENT:        return "'--'";
        case TOKEN_EQ:               return "'=='";
        case TOKEN_NE:               return "'!='";
        case TOKEN_LT:               return "'<'";
        case TOKEN_LE:               return "'<='";
        case TOKEN_GT:               return "'>'";
        case TOKEN_GE:               return "'>='";
        case TOKEN_LEFT_SHIFT:       return "'<<'";
        case TOKEN_RIGHT_SHIFT:      return "'>>'";
        case TOKEN_AND_AND:          return "'&&'";
        case TOKEN_OR_OR:            return "'||'";
        case TOKEN_AMPERSAND:        return "'&'";
        case TOKEN_CARET:            return "'^'";
        case TOKEN_PIPE:             return "'|'";
        case TOKEN_QUESTION:         return "'?'";
        case TOKEN_DOT:              return "'.'";
        case TOKEN_ARROW:            return "'->'";
        case TOKEN_ELLIPSIS:         return "'...'";
        case TOKEN_UNKNOWN:          return "<unknown>";
    }
    return "<unknown>";
}
