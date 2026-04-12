
Step 1: Tokenizer Changes

What's changing and why:

The tokenizer currently handles = only as the start of ==. We need to add a new token type TOKEN_ASSIGN for a standalone =. The lexer already peeks at
the next character when it sees =, so we just need to add a fallthrough case: if = is not followed by =, emit TOKEN_ASSIGN instead of falling through
to TOKEN_UNKNOWN.

Here are the two changes:

1. Add TOKEN_ASSIGN to the TokenType enum
2. Add a fallthrough in the lexer for standalone =