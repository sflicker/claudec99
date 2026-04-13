  ---                                                                                                                                                   
Summary of AST + Parser changes

1. AST: Added AST_WHILE_STATEMENT to the ASTNodeType enum
2. parse_while_statement(): New function that parses while ( <expr> ) <stmt> — stores condition as children[0] and body as children[1]
3. parse_statement(): Added TOKEN_WHILE check that dispatches to parse_while_statement()                                                              
                                                                            