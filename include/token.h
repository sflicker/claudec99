#ifndef CCOMPILER_TOKEN_H
#define CCOMPILER_TOKEN_H

#include <stddef.h>
#include "type.h"

typedef struct SourceFile {
    char *path;
} SourceFile;

typedef enum {
    TOKEN_EOF,
    TOKEN_INT,
    TOKEN_CHAR,
    TOKEN_SHORT,
    TOKEN_LONG,
    TOKEN_RETURN,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_DO,
    TOKEN_FOR,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_SWITCH,
    TOKEN_DEFAULT,
    TOKEN_CASE,
    TOKEN_GOTO,
    TOKEN_SIZEOF,
    TOKEN_EXTERN,
    TOKEN_STATIC,
    TOKEN_TYPEDEF,
    TOKEN_ENUM,
    TOKEN_STRUCT,
    TOKEN_UNION,
    TOKEN_VOID,
    TOKEN_BOOL,
    TOKEN_CONST,
    TOKEN_VOLATILE,
    TOKEN_RESTRICT,
    TOKEN_INLINE,
    TOKEN_SIGNED,
    TOKEN_UNSIGNED,
    TOKEN_IDENTIFIER,
    TOKEN_INT_LITERAL,
    TOKEN_STRING_LITERAL,
    TOKEN_CHAR_LITERAL,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_SEMICOLON,
    TOKEN_COLON,
    TOKEN_COMMA,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PERCENT,
    TOKEN_BANG,
    TOKEN_TILDE,
    TOKEN_ASSIGN,
    TOKEN_PLUS_ASSIGN,
    TOKEN_MINUS_ASSIGN,
    TOKEN_STAR_ASSIGN,
    TOKEN_SLASH_ASSIGN,
    TOKEN_PERCENT_ASSIGN,
    TOKEN_LEFT_SHIFT_ASSIGN,
    TOKEN_RIGHT_SHIFT_ASSIGN,
    TOKEN_AMP_ASSIGN,
    TOKEN_CARET_ASSIGN,
    TOKEN_PIPE_ASSIGN,
    TOKEN_INCREMENT,
    TOKEN_DECREMENT,
    TOKEN_EQ,
    TOKEN_NE,
    TOKEN_LT,
    TOKEN_LE,
    TOKEN_GT,
    TOKEN_GE,
    TOKEN_LEFT_SHIFT,
    TOKEN_RIGHT_SHIFT,
    TOKEN_AND_AND,
    TOKEN_OR_OR,
    TOKEN_AMPERSAND,
    TOKEN_CARET,
    TOKEN_PIPE,
    TOKEN_QUESTION,
    TOKEN_DOT,
    TOKEN_ARROW,
    TOKEN_ELLIPSIS,
    TOKEN_UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    /* Stage 95-08: raw source spelling (owned by lexer). */
    const char *lexeme;
    size_t lexeme_len;
    /* Stage 95-08: decoded/cooked value (owned by lexer). For string
     * literals this is the decoded byte sequence; for other tokens it
     * is the same text as the source spelling. value is always
     * null-terminated as a convenience sentinel; use value_len for the
     * true byte count (required for embedded-NUL string literals). */
    const char *value;
    size_t value_len;
    long long_value;
    TypeKind literal_type;
    /* Stage 00-98: set when the integer literal has a U/u suffix. */
    int is_unsigned;
    /* Stage 70-02: source position (1-based). */
    int line;
    int col;
    SourceFile *file;  /* not owned by token; points into lexer's file pool */
} Token;

const char *token_display_name(TokenType type);

#endif
