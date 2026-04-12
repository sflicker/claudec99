  -  AST + Parser Changes Summary

  AST (Section 2)

  ┌──────────────────────────────────────────────────────────────────────┬─────────────────────────────────────────┐                                                                 
  │                                Change                                │                   Why                   │                                                                 
  ├──────────────────────────────────────────────────────────────────────┼─────────────────────────────────────────┤                                                                 
  │ Added AST_UNARY_OP, AST_IF_STATEMENT, AST_BLOCK, AST_EXPRESSION_STMT │ New node types required by spec         │                                                                 
  ├──────────────────────────────────────────────────────────────────────┼─────────────────────────────────────────┤                                                                 
  │ children[4] → children[AST_MAX_CHILDREN] (64)                        │ Blocks need variable number of children │                                                                 
  └──────────────────────────────────────────────────────────────────────┴─────────────────────────────────────────┘

  Parser (Section 4)

  ┌──────────────────────────────────────────────────────┬──────────────────────────────────────────────────────────────────────────────────────┐                                    
  │                        Change                        │                                         Why                                          │
  ├──────────────────────────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────┤
  │ parse_factor → parse_primary                         │ Renamed to match spec grammar                                                        │
  ├──────────────────────────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────┤
  │ New parse_unary                                      │ Handles +, -, ! prefix operators; recurses for chaining (e.g. --5, !!x)              │                                    
  ├──────────────────────────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────┤                                    
  │ parse_term calls parse_unary instead of parse_factor │ Unary sits between term and primary in precedence                                    │                                    
  ├──────────────────────────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────┤                                    
  │ New parse_block                                      │ Parses { stmt* }, creates AST_BLOCK with children                                    │
  ├──────────────────────────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────┤                                    
  │ New parse_if_statement                               │ Parses if (expr) stmt [else stmt], creates AST_IF_STATEMENT with 2-3 children        │
  ├──────────────────────────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────┤                                    
  │ parse_statement expanded                             │ Dispatches on token: return → return_stmt, if → if_stmt, { → block, else → expr_stmt │
  ├──────────────────────────────────────────────────────┼──────────────────────────────────────────────────────────────────────────────────────┤                                    
  │ parse_function_decl uses parse_block                 │ Function body is now <block> per grammar                                             │
  └──────────────────────────────────────────────────────┴──────────────────────────────────────────────────────────────────────────────────────┘

  Key design choices

  - AST_IF_STATEMENT uses 2 children (no else) or 3 children (with else) — child_count distinguishes them
  - parse_statement for return consumes the return token directly (avoiding double-dispatch), consistent with how the if and block dispatchers work
  - else binds to nearest if naturally via recursive descent

  Ready to proceed to code generation when you confirm.                            