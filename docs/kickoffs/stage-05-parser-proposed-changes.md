Step 2: AST + Parser Changes

The AST and parser are tightly coupled — the parser creates AST nodes — so I'll cover both together.

What's changing and why

AST (3 new node types):
- AST_DECLARATION — represents int x; or int x = 5;. Value stores the variable name, optional child is the initializer expression.
- AST_ASSIGNMENT — represents x = 7;. Value stores the variable name, one child is the RHS expression.
- AST_VAR_REF — represents a variable used in an expression (e.g., return x;). Value stores the variable name.

Parser changes:
1. parse_primary — handle TOKEN_IDENTIFIER by producing an AST_VAR_REF node
2. parse_statement — handle two new cases:                                                                                                            
   - TOKEN_INT (inside a block) → parse as declaration                                                                                                 
   - TOKEN_IDENTIFIER followed by TOKEN_ASSIGN → parse as assignment                                                                                   
   - TOKEN_IDENTIFIER NOT followed by TOKEN_ASSIGN → falls through to expression statement (the identifier becomes a var ref via parse_primary)

The tricky part is distinguishing x = 5; (assignment) from x + 1; (expression statement starting with a variable). I'll use a save/restore approach on
the lexer to peek one token ahead.                                        