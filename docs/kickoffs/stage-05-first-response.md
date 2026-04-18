Good, I now have a complete picture. Here is my summary and the first step.
                                                                                                                                      
---                                                                                                                                                   
Stage 5 Summary: Integer Variables

What's new in Stage 5 (compared to Stage 4):

1. New token: TOKEN_ASSIGN (=) — a single = for assignment, distinct from ==
2. New AST node types:                                                                                                                                
   - AST_DECLARATION — int x; or int x = 5;                                                                                                            
   - AST_ASSIGNMENT — x = 7;                                                                                                                           
   - AST_VAR_REF — variable reference in an expression (e.g., return x;)
3. Parser changes:                                                                                                                                    
   - parse_statement must handle declarations (int ...) and assignments (identifier = ...)                                                             
   - parse_primary must handle identifiers as variable references
4. Code generation:                                                                                                                                   
   - Stack frame setup (push rbp / mov rbp, rsp / sub rsp, N)                                                                                          
   - Per-function symbol table mapping variable names to [rbp - offset]                                                                                
   - Declaration allocates stack space, optionally stores initial value                                                                                
   - Assignment stores value into stack slot                                                                                                           
   - Variable reference loads from stack slot
5. Semantic errors:                                                                                                                                   
   - Undeclared variable usage/assignment                                                                                                            
   - Duplicate declaration in same scope                                                                                                               
   - Variables only allowed in function's outermost block (not nested blocks)

  ---       
