Summary of Parser + AST Changes

AST (3 new node types added to enum):
- AST_DECLARATION — value = variable name, 0 children (uninitialized) or 1 child (initializer expression)
- AST_ASSIGNMENT — value = variable name, 1 child (RHS expression)
- AST_VAR_REF — value = variable name, 0 children (leaf node in expressions)

Parser changes:

1. parse_primary — added TOKEN_IDENTIFIER case that creates an AST_VAR_REF node. This allows variables to appear anywhere in expressions (e.g., a + b,
   a * (a + 1)).
2. parse_statement — added two new cases at the top:                                                                                                  
   - Declaration (TOKEN_INT): consumes int, identifier, optional = <expr>, and ;. Creates AST_DECLARATION.                                             
   - Assignment (TOKEN_IDENTIFIER + peek for TOKEN_ASSIGN): uses save/restore on the lexer position to peek one token ahead. If = follows the          
   identifier, it's an assignment. Otherwise, restores state and falls through to expression statement parsing.

The save/restore peek approach avoids adding a lookahead buffer to the parser while keeping the grammar unambiguous.
                                                                                                                                                      
---                                                                                                                                                   
Ready for Step 3: Code Generation (symbol table, stack allocation, variable load/store) when you give the go-ahead.            