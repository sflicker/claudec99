● Milestone Summary                                                                                                                                                      
                                                                                                                                                                         
  Stage-14-04: String Literals as char * Expressions — Complete                                                                                                          
                                                                                                                                                                         
  String literals now decay to char * in expression contexts, enabling char *s = "hi"; initialization, s = "hi"; assignment, s[i] indexing, and *(s + 1) pointer         
  arithmetic. Cross-base initializers like int *p = "hello"; are correctly rejected.                                                                                     
                                                                                                                                                                         
  Codegen — two surgical edits in src/codegen.c: the AST_STRING_LITERAL branch sets result_type=TYPE_POINTER, full_type=type_pointer(type_char());                       
  codegen_emit_string_pool derives byte length from strlen(node->value).                                                                                                 
                                                                                                                                                                         
  Tests — added 6 valid + 1 invalid test from the spec; refreshed the three Stage 14-03 print_asm snapshots to use a char *s = "..."; return 0; form so they survive the 
  stricter return-type check.                                                                                                                                          
                                                                                                                                                                         
  Build & tests: clean. 379 / 379 pass (238 valid + 45 invalid + 21 print-AST + 72 print-tokens + 3 print-asm). No regressions.         

