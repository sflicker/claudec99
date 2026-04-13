Summary of tokenizer changes

1. TOKEN_WHILE added to the TokenType enum at line 24 (between TOKEN_ELSE and TOKEN_IDENTIFIER)
2. Keyword recognition added at line 189 — when the lexer reads while, it returns TOKEN_WHILE instead of TOKEN_IDENTIFIER

These are the only two changes needed for the tokenizer. Ready for your go-ahead to proceed to Step 2 (Parser + AST).              