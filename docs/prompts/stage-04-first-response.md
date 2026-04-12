Stage 4 Summary

The spec requires adding these features to the stage 3 compiler:

New Tokens

- if keyword
- else keyword
- ! (logical NOT, standalone — distinct from !=)
- identifier (already exists from stage 3)

Grammar Changes

1. <block> — new production: "{" { <statement> } "}" (zero or more statements in braces)                                                                                                                     
2. <statement> — expanded from just return to include: return_statement | if_statement | block | expression_stmt
3. <if_statement> — new: "if" "(" <expression> ")" <statement> [ "else" <statement> ]
4. <expression_stmt> — new: <expression> ";"
5. <unary_expr> — new precedence level between <term> and <primary>: [ "+" | "-" | "!" ] <unary_expr> | <primary>
6. <function> — body becomes a <block> instead of a single <statement>                                                                                                                                       

AST Changes

- New AST_UNARY_OP node type (operator + child expression)
- New AST_IF_STATEMENT node type
- New AST_BLOCK node type
- New AST_EXPRESSION_STMT node type

Semantics

- ! returns 1 if operand is 0, otherwise 0
- Unary - negates, unary + is identity
- if uses integer truthiness (0 = false, nonzero = true)
- else binds to nearest unmatched if

Note on test_unary_neg__-42.c

The filename expects exit code -42, but Linux exit codes are 0-255 (-42 wraps to 214). The test runner compares 214 -eq -42 which will fail. We may need to adjust that test filename after implementation,  
but I'll flag it then.
                                                                                                                                                                                                               
---             
Step 1: Tokenizer Updates

Three changes needed:

1. Add TOKEN_IF, TOKEN_ELSE, TOKEN_BANG to the TokenType enum
2. Handle standalone ! — currently ! only works as part of !=. We need ! alone to produce TOKEN_BANG
3. Add if/else keyword matching in the keyword section of lexer_next_token

Here are the changes:

Change 1: Add new token types to the enum                                    