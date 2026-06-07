#ifndef CCOMPILER_LEXER_H
#define CCOMPILER_LEXER_H

#include "token.h"
#include "vec.h"

typedef struct {
    const char *source;
    int pos;
    int line;
    int col;
    SourceFile *file;       /* current file (points into file_pool) */
    SourceFile **file_pool; /* owned array of heap-allocated SourceFile* */
    int n_files;
    int files_cap;
    /* Stage 95-08: pool of heap-allocated strings owned by the lexer.
     * Each element is a char* allocated by lexer_store_bytes; all are
     * freed in lexer_free.  Tokens hold const char* pointers into this
     * pool, which remain stable even as the pool Vec grows. */
    Vec str_pool;           /* char* elements */
} Lexer;

/* Initialize lexer. filename is the logical path of the source (may be NULL).
 * All SourceFile objects are owned by the Lexer; call lexer_free when done. */
void lexer_init(Lexer *lexer, const char *source, const char *filename);
void lexer_free(Lexer *lexer);
Token lexer_next_token(Lexer *lexer);

#endif
