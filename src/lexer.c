#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"

void lexer_init(Lexer *lexer, const char *source) {
    lexer->source = source;
    lexer->pos = 0;
}

static void lexer_skip_whitespace(Lexer *lexer) {
    while (lexer->source[lexer->pos] && isspace(lexer->source[lexer->pos])) {
        lexer->pos++;
    }
}

Token lexer_next_token(Lexer *lexer) {
    Token token = {TOKEN_EOF, ""};

    lexer_skip_whitespace(lexer);

    char c = lexer->source[lexer->pos];
    if (c == '\0') {
        return token;
    }

    /* Single-character tokens */
    if (c == '(') { token.type = TOKEN_LPAREN;    token.value[0] = c; lexer->pos++; return token; }
    if (c == ')') { token.type = TOKEN_RPAREN;    token.value[0] = c; lexer->pos++; return token; }
    if (c == '{') { token.type = TOKEN_LBRACE;    token.value[0] = c; lexer->pos++; return token; }
    if (c == '}') { token.type = TOKEN_RBRACE;    token.value[0] = c; lexer->pos++; return token; }
    if (c == ';') { token.type = TOKEN_SEMICOLON; token.value[0] = c; lexer->pos++; return token; }
    if (c == ':') { token.type = TOKEN_COLON;     token.value[0] = c; lexer->pos++; return token; }
    if (c == ',') { token.type = TOKEN_COMMA;     token.value[0] = c; lexer->pos++; return token; }
    if (c == '+') {
        if (lexer->source[lexer->pos + 1] == '+') { token.type = TOKEN_INCREMENT; strcpy(token.value, "++"); lexer->pos += 2; return token; }
        if (lexer->source[lexer->pos + 1] == '=') { token.type = TOKEN_PLUS_ASSIGN; strcpy(token.value, "+="); lexer->pos += 2; return token; }
        token.type = TOKEN_PLUS; token.value[0] = c; lexer->pos++; return token;
    }
    if (c == '-') {
        if (lexer->source[lexer->pos + 1] == '-') { token.type = TOKEN_DECREMENT; strcpy(token.value, "--"); lexer->pos += 2; return token; }
        if (lexer->source[lexer->pos + 1] == '=') { token.type = TOKEN_MINUS_ASSIGN; strcpy(token.value, "-="); lexer->pos += 2; return token; }
        token.type = TOKEN_MINUS; token.value[0] = c; lexer->pos++; return token;
    }
    if (c == '*') { token.type = TOKEN_STAR;       token.value[0] = c; lexer->pos++; return token; }
    if (c == '/') { token.type = TOKEN_SLASH;      token.value[0] = c; lexer->pos++; return token; }

    /* Two-character or single-character relational/equality tokens */
    char n = lexer->source[lexer->pos + 1];
    if (c == '=' && n == '=') { token.type = TOKEN_EQ; strcpy(token.value, "=="); lexer->pos += 2; return token; }
    if (c == '=') { token.type = TOKEN_ASSIGN; token.value[0] = c; lexer->pos++; return token; }
    if (c == '!' && n == '=') { token.type = TOKEN_NE; strcpy(token.value, "!="); lexer->pos += 2; return token; }
    if (c == '!') { token.type = TOKEN_BANG; token.value[0] = c; lexer->pos++; return token; }
    if (c == '<' && n == '=') { token.type = TOKEN_LE; strcpy(token.value, "<="); lexer->pos += 2; return token; }
    if (c == '>' && n == '=') { token.type = TOKEN_GE; strcpy(token.value, ">="); lexer->pos += 2; return token; }
    if (c == '<') { token.type = TOKEN_LT; token.value[0] = c; lexer->pos++; return token; }
    if (c == '>') { token.type = TOKEN_GT; token.value[0] = c; lexer->pos++; return token; }
    if (c == '&' && n == '&') { token.type = TOKEN_AND_AND; strcpy(token.value, "&&"); lexer->pos += 2; return token; }
    if (c == '|' && n == '|') { token.type = TOKEN_OR_OR;   strcpy(token.value, "||"); lexer->pos += 2; return token; }

    /* Integer literals */
    if (isdigit(c)) {
        int i = 0;
        while (isdigit(lexer->source[lexer->pos]) && i < 255) {
            token.value[i++] = lexer->source[lexer->pos++];
        }
        token.value[i] = '\0';
        token.type = TOKEN_INT_LITERAL;
        token.int_value = atoi(token.value);
        return token;
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
        } else {
            token.type = TOKEN_IDENTIFIER;
        }
        return token;
    }

    /* Unknown token */
    token.type = TOKEN_UNKNOWN;
    token.value[0] = c;
    lexer->pos++;
    return token;
}
