● All 6 tests pass. Here's a summary of what was implemented:                                                                                                                                   
                                                                                          
  Summary                                                                                                                                                                                       
                                                                                                                                                                                                
  Tokenizer (compiler.c:28-32, 123-126, 136)                                                                                                                                                    
                                                                                                                                                                                                
  - Added TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH token types                                                                                                                          
  - Added int_value field to Token struct, set via atoi() for integer literals                                                                                                                
  - Lexer recognizes +, -, *, / as single-character tokens                                                                                                                                      
                                                                                                                                                                                                
  AST (compiler.c:50)                                                                                                                                                                           
                                                                                                                                                                                                
  - Added AST_BINARY_OP node type                                                                                                                                                               
  - Binary nodes store the operator character in value, left operand in children[0], right operand in children[1]                                                                             
                                                                                                                                                                                                
  Parser (compiler.c:193-245)
                                                                                                                                                                                                
  - parse_factor — handles int literals and parenthesized expressions                                                                                                                           
  - parse_term — handles * and / with left associativity (higher precedence)
  - parse_expression — handles + and - with left associativity (lower precedence)                                                                                                               
                                                                                                                                                                                                
  Code Generation (compiler.c:301-331)                                                                                                                                                          
                                                                                                                                                                                                
  - Binary ops: evaluate left → push → evaluate right → pop left into ecx                                                                                                                       
  - +: add eax, ecx                                                                                                                                                                           
  - -: sub ecx, eax; mov eax, ecx (preserves left-right order)                                                                                                                                  
  - *: imul eax, ecx                                                                                                                                                                            
  - /: xchg eax, ecx; cdq; idiv ecx (divides left by right)                                                                                                                                     
                                                                                                                                                                                                
  Test Script (test/run_tests.sh)                                                                                                                                                               
                                                                                                                                                                                                
  Compiles, assembles, links, runs each test/*.c file and verifies exit code == 42.        
