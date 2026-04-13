● I'll start by reading the stage 6 spec and understanding the current codebase structure.

Searched for 1 pattern, read 3 files (ctrl+o to expand)

● Stage 6 Summary

The spec requires adding while loop support. Here's what needs to change:

1. Tokenizer: Add TOKEN_WHILE keyword
2. AST: Add AST_WHILE_STATEMENT node type
3. Parser: Add parse_while_statement(), wire it into parse_statement()
4. Codegen: Emit loop-start label, condition check, conditional jump to end, body, jump back to start

The while statement stores two children: the condition expression and the body statement. A test file (test_while_loop__42.c) already exists.
                                                                                                                                                        
---                                                                                                                                                   
Step 1: Tokenizer

We need to add TOKEN_WHILE to the enum and recognize the while keyword in the lexer.

Two changes:
1. Add TOKEN_WHILE to the TokenType enum (after TOKEN_ELSE, before TOKEN_IDENTIFIER)
2. Add an else if branch in the keyword recognition block of lexer_next_token                           