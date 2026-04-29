#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"

void lexer_init(Lexer *lexer, const char *source) {
    lexer->source = source;
    lexer->pos = 0;
}

static void lexer_skip_whitespace(Lexer *lexer) {
    for (;;) {
        while (lexer->source[lexer->pos] && isspace(lexer->source[lexer->pos])) {
            lexer->pos++;
        }
        if (lexer->source[lexer->pos] == '/' && lexer->source[lexer->pos + 1] == '/') {
            lexer->pos += 2;
            while (lexer->source[lexer->pos] && lexer->source[lexer->pos] != '\n') {
                lexer->pos++;
            }
            continue;
        }
        if (lexer->source[lexer->pos] == '/' && lexer->source[lexer->pos + 1] == '*') {
            lexer->pos += 2;
            while (lexer->source[lexer->pos] &&
                   !(lexer->source[lexer->pos] == '*' && lexer->source[lexer->pos + 1] == '/')) {
                lexer->pos++;
            }
            if (lexer->source[lexer->pos] == '*' && lexer->source[lexer->pos + 1] == '/') {
                lexer->pos += 2;
            }
            continue;
        }
        break;
    }
}

/* Set token.length from token.value (NUL-terminated source-byte text)
 * and return. Used by every non-string-literal branch; string literals
 * set token.length explicitly because their body may eventually contain
 * embedded NUL bytes once escape sequences are supported. */
static Token finalize(Token token) {
    token.length = (int)strlen(token.value);
    return token;
}

Token lexer_next_token(Lexer *lexer) {
    Token token = {TOKEN_EOF, ""};

    lexer_skip_whitespace(lexer);

    char c = lexer->source[lexer->pos];
    if (c == '\0') {
        return finalize(token);
    }

    /* Single-character tokens */
    if (c == '(') { token.type = TOKEN_LPAREN;    token.value[0] = c; lexer->pos++; return finalize(token); }
    if (c == ')') { token.type = TOKEN_RPAREN;    token.value[0] = c; lexer->pos++; return finalize(token); }
    if (c == '{') { token.type = TOKEN_LBRACE;    token.value[0] = c; lexer->pos++; return finalize(token); }
    if (c == '}') { token.type = TOKEN_RBRACE;    token.value[0] = c; lexer->pos++; return finalize(token); }
    if (c == '[') { token.type = TOKEN_LBRACKET;  token.value[0] = c; lexer->pos++; return finalize(token); }
    if (c == ']') { token.type = TOKEN_RBRACKET;  token.value[0] = c; lexer->pos++; return finalize(token); }
    if (c == ';') { token.type = TOKEN_SEMICOLON; token.value[0] = c; lexer->pos++; return finalize(token); }
    if (c == ':') { token.type = TOKEN_COLON;     token.value[0] = c; lexer->pos++; return finalize(token); }
    if (c == ',') { token.type = TOKEN_COMMA;     token.value[0] = c; lexer->pos++; return finalize(token); }
    if (c == '+') {
        if (lexer->source[lexer->pos + 1] == '+') { token.type = TOKEN_INCREMENT; strcpy(token.value, "++"); lexer->pos += 2; return finalize(token); }
        if (lexer->source[lexer->pos + 1] == '=') { token.type = TOKEN_PLUS_ASSIGN; strcpy(token.value, "+="); lexer->pos += 2; return finalize(token); }
        token.type = TOKEN_PLUS; token.value[0] = c; lexer->pos++; return finalize(token);
    }
    if (c == '-') {
        if (lexer->source[lexer->pos + 1] == '-') { token.type = TOKEN_DECREMENT; strcpy(token.value, "--"); lexer->pos += 2; return finalize(token); }
        if (lexer->source[lexer->pos + 1] == '=') { token.type = TOKEN_MINUS_ASSIGN; strcpy(token.value, "-="); lexer->pos += 2; return finalize(token); }
        token.type = TOKEN_MINUS; token.value[0] = c; lexer->pos++; return finalize(token);
    }
    if (c == '*') { token.type = TOKEN_STAR;       token.value[0] = c; lexer->pos++; return finalize(token); }
    if (c == '/') { token.type = TOKEN_SLASH;      token.value[0] = c; lexer->pos++; return finalize(token); }

    /* Two-character or single-character relational/equality tokens */
    char n = lexer->source[lexer->pos + 1];
    if (c == '=' && n == '=') { token.type = TOKEN_EQ; strcpy(token.value, "=="); lexer->pos += 2; return finalize(token); }
    if (c == '=') { token.type = TOKEN_ASSIGN; token.value[0] = c; lexer->pos++; return finalize(token); }
    if (c == '!' && n == '=') { token.type = TOKEN_NE; strcpy(token.value, "!="); lexer->pos += 2; return finalize(token); }
    if (c == '!') { token.type = TOKEN_BANG; token.value[0] = c; lexer->pos++; return finalize(token); }
    if (c == '<' && n == '=') { token.type = TOKEN_LE; strcpy(token.value, "<="); lexer->pos += 2; return finalize(token); }
    if (c == '>' && n == '=') { token.type = TOKEN_GE; strcpy(token.value, ">="); lexer->pos += 2; return finalize(token); }
    if (c == '<') { token.type = TOKEN_LT; token.value[0] = c; lexer->pos++; return finalize(token); }
    if (c == '>') { token.type = TOKEN_GT; token.value[0] = c; lexer->pos++; return finalize(token); }
    if (c == '&' && n == '&') { token.type = TOKEN_AND_AND; strcpy(token.value, "&&"); lexer->pos += 2; return finalize(token); }
    if (c == '&') { token.type = TOKEN_AMPERSAND; token.value[0] = c; lexer->pos++; return finalize(token); }
    if (c == '|' && n == '|') { token.type = TOKEN_OR_OR;   strcpy(token.value, "||"); lexer->pos += 2; return finalize(token); }

    /* String literals: double-quoted, ordinary characters only.
     * Body bytes are stored in token.value (without the surrounding
     * quotes) and token.length records the byte count. Three rejection
     * cases:
     *   - newline byte before the closing quote
     *   - end-of-file before the closing quote
     *   - any backslash escape sequence (not yet supported)
     * Body length is capped at 255 bytes to fit token.value[256]. */
    if (c == '"') {
        lexer->pos++;
        int i = 0;
        for (;;) {
            char ch = lexer->source[lexer->pos];
            if (ch == '"') {
                lexer->pos++;
                break;
            }
            if (ch == '\0') {
                fprintf(stderr,
                        "error: unterminated string literal\n");
                exit(1);
            }
            if (ch == '\n') {
                fprintf(stderr,
                        "error: newline in string literal\n");
                exit(1);
            }
            if (ch == '\\') {
                fprintf(stderr,
                        "error: escape sequences not supported in string literals\n");
                exit(1);
            }
            if (i >= 255) {
                fprintf(stderr,
                        "error: string literal too long (max 255 bytes)\n");
                exit(1);
            }
            token.value[i++] = ch;
            lexer->pos++;
        }
        token.value[i] = '\0';
        token.length = i;
        token.type = TOKEN_STRING_LITERAL;
        return token;
    }

    /* Integer literals: digits, optional 'L' or 'l' suffix forces long.
     * Without a suffix, the type is int when the value fits in a signed
     * 32-bit int and long otherwise. Values that exceed the long range
     * are rejected as "too large for supported integer types". */
    if (isdigit(c)) {
        int i = 0;
        while (isdigit(lexer->source[lexer->pos]) && i < 255) {
            token.value[i++] = lexer->source[lexer->pos++];
        }
        token.value[i] = '\0';
        int has_long_suffix = 0;
        if (lexer->source[lexer->pos] == 'L' || lexer->source[lexer->pos] == 'l') {
            has_long_suffix = 1;
            lexer->pos++;
        }
        errno = 0;
        char *end = NULL;
        unsigned long parsed = strtoul(token.value, &end, 10);
        if (errno == ERANGE || parsed > (unsigned long)LONG_MAX) {
            fprintf(stderr,
                    "error: integer literal '%s' too large for supported integer types\n",
                    token.value);
            exit(1);
        }
        token.long_value = (long)parsed;
        if (has_long_suffix) {
            token.literal_type = TYPE_LONG;
        } else if (parsed <= (unsigned long)INT_MAX) {
            token.literal_type = TYPE_INT;
        } else {
            token.literal_type = TYPE_LONG;
        }
        token.type = TOKEN_INT_LITERAL;
        return finalize(token);
    }

    /* Keywords and identifiers */
    if (isalpha(c) || c == '_') {
        int i = 0;
        while ((isalnum(lexer->source[lexer->pos]) || lexer->source[lexer->pos] == '_') && i < 255) {
            token.value[i++] = lexer->source[lexer->pos++];
        }
        token.value[i] = '\0';

        if (strcmp(token.value, "int") == 0) {
            token.type = TOKEN_INT;
        } else if (strcmp(token.value, "char") == 0) {
            token.type = TOKEN_CHAR;
        } else if (strcmp(token.value, "short") == 0) {
            token.type = TOKEN_SHORT;
        } else if (strcmp(token.value, "long") == 0) {
            token.type = TOKEN_LONG;
        } else if (strcmp(token.value, "return") == 0) {
            token.type = TOKEN_RETURN;
        } else if (strcmp(token.value, "if") == 0) {
            token.type = TOKEN_IF;
        } else if (strcmp(token.value, "else") == 0) {
            token.type = TOKEN_ELSE;
        } else if (strcmp(token.value, "while") == 0) {
            token.type = TOKEN_WHILE;
        } else if (strcmp(token.value, "do") == 0) {
            token.type = TOKEN_DO;
        } else if (strcmp(token.value, "for") == 0) {
            token.type = TOKEN_FOR;
        } else if (strcmp(token.value, "break") == 0) {
            token.type = TOKEN_BREAK;
        } else if (strcmp(token.value, "continue") == 0) {
            token.type = TOKEN_CONTINUE;
        } else if (strcmp(token.value, "switch") == 0) {
            token.type = TOKEN_SWITCH;
        } else if (strcmp(token.value, "default") == 0) {
            token.type = TOKEN_DEFAULT;
        } else if (strcmp(token.value, "case") == 0) {
            token.type = TOKEN_CASE;
        } else if (strcmp(token.value, "goto") == 0) {
            token.type = TOKEN_GOTO;
        } else {
            token.type = TOKEN_IDENTIFIER;
        }
        return finalize(token);
    }

    /* Unknown token */
    token.type = TOKEN_UNKNOWN;
    token.value[0] = c;
    lexer->pos++;
    return finalize(token);
}
