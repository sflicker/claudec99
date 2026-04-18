● Let me read the spec and existing code first.

  Searched for 1 pattern, read 8 files (ctrl+o to expand)                                                                                                                                       
                                                              
● Now I have the full picture. Here's my plan, then the implementation.                                                                                                                         
                                                                                                                                                                                              
  Implementation Plan                                                                                                                                                                           
                                                                                                                                                                                                
  Tokenizer: Add TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH. Add int_value field to Token.                                                                                                
                                                                                                                                                                                                
  AST: Add AST_BINARY_OP type. Binary nodes store the operator in value, with left/right as children[0]/children[1].                                                                            
                  
  Parser (recursive descent for precedence):                                                                                                                                                    
  - parse_expression(Parser*) → handles +, - (lowest precedence)
  - parse_term(Parser*) → handles *, /                                                                                                                                                          
  - parse_factor(Parser*) → handles int literals and (expr)
                                                                                                                                                                                                
  Code gen: Evaluate left into eax, push it, evaluate right into eax, pop left into ecx, perform op. Division uses cdq + idiv.                                                                  
                                                                                                                                                   

