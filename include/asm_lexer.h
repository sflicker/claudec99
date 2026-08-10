#ifndef CCOMPILER_ASM_LEXER_H
#define CCOMPILER_ASM_LEXER_H

#include <stddef.h>

/* Stage 177: token kinds for the NASM-subset assembler lexer. Comments
 * (";" to end of line) and whitespace are consumed silently and never
 * produce a token. Newlines ARE significant: each source line becomes
 * exactly one AsmLine in the parser, so ASM_TOK_NEWLINE is the line
 * terminator the parser scans for. */
typedef enum {
    ASM_TOK_EOF,
    ASM_TOK_NEWLINE,
    ASM_TOK_IDENT,      /* label / mnemonic / directive / register / symbol */
    ASM_TOK_INT,        /* decimal or 0x-hex integer literal (unsigned
                          * magnitude; a leading "-" lexes as ASM_TOK_MINUS
                          * and is folded into the value by the parser) */
    ASM_TOK_LBRACKET,
    ASM_TOK_RBRACKET,
    ASM_TOK_PLUS,
    ASM_TOK_MINUS,
    ASM_TOK_COMMA,
    ASM_TOK_COLON,
    ASM_TOK_UNKNOWN
} AsmTokenType;

typedef struct {
    AsmTokenType type;
    char        *text;   /* heap-owned spelling; only set for ASM_TOK_IDENT */
    long         value;  /* decoded value; only set for ASM_TOK_INT */
    int          line;   /* 1-based source line */
    int          col;    /* 1-based source column */
} AsmToken;

typedef struct {
    const char *src;   /* full source buffer, not owned by the lexer */
    size_t      pos;
    int         line;
    int         col;
} AsmLexer;

void     asm_lexer_init(AsmLexer *lex, const char *src);
AsmToken asm_lexer_next(AsmLexer *lex);
void     asm_token_free(AsmToken *tok);

/* Small standalone helpers, used by casm.c and asm_parser.c. Deliberately
 * not shared with src/util.c -- see the Architecture note in
 * docs/stages/ClaudeC99-spec-stage-177-assembler-lexer-parser.md. */
char *asm_read_file(const char *path);
char *asm_strdup(const char *s);

#endif /* CCOMPILER_ASM_LEXER_H */
