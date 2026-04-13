● All 42 tests pass (40 valid + 2 invalid).

Summary of codegen changes

Added AST_WHILE_STATEMENT handling in codegen_statement() (after the AST_IF_STATEMENT block):

.L_while_start_N:          ; loop start label                                                                                                         
<evaluate condition>                                                                                                                              
cmp eax, 0                                                                                                                                        
je .L_while_end_N      ; if false, exit loop                                                                                                      
<emit body>                                                                                                                                       
jmp .L_while_start_N   ; jump back to condition                                                                                                 
.L_while_end_N:            ; loop exit

Uses the same label_count mechanism as if statements for unique label IDs.
                                                                                                                                                      
---                                                                                                                                                   
Stage 6 is now complete. The spec also mentions "at least one new test has been added" — test_while_loop__42.c already existed. Would you like me to
add any additional while loop tests (e.g., while with condition initially false, nested while, while with a single non-block statement)?
