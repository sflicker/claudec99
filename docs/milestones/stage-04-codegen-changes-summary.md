Code Generation Changes Summary

┌──────────────────────────────────────────┬─────────────────────────────────────────────────────────────────────────────────────┐                                                 
│                  Change                  │                                         Why                                         │                                                 
├──────────────────────────────────────────┼─────────────────────────────────────────────────────────────────────────────────────┤                                                 
│ Added label_count to CodeGen             │ Unique labels for if/else jump targets                                              │
├──────────────────────────────────────────┼─────────────────────────────────────────────────────────────────────────────────────┤
│ AST_UNARY_OP in codegen_expression       │ - → neg eax; ! → cmp eax,0; sete al; movzx eax,al; + → no-op                        │                                                 
├──────────────────────────────────────────┼─────────────────────────────────────────────────────────────────────────────────────┤                                                 
│ AST_IF_STATEMENT in codegen_statement    │ Evaluates condition, je to else/end label, generates then/else branches with labels │                                                 
├──────────────────────────────────────────┼─────────────────────────────────────────────────────────────────────────────────────┤                                                 
│ AST_BLOCK in codegen_statement           │ Iterates over children, generates each statement                                    │                                               
├──────────────────────────────────────────┼─────────────────────────────────────────────────────────────────────────────────────┤                                                 
│ AST_EXPRESSION_STMT in codegen_statement │ Evaluates expression (result discarded)                                             │
├──────────────────────────────────────────┼─────────────────────────────────────────────────────────────────────────────────────┤                                                 
│ Renamed test_unary_neg__-42.c → __214.c  │ Linux exit codes are 0-255; -42 & 0xFF = 214                                        │
└──────────────────────────────────────────┴─────────────────────────────────────────────────────────────────────────────────────┘

Generated assembly example (test_basic_if__42.c):

- if(1) → mov eax, 1; cmp eax, 0; je .L_else_0 — jumps over the then-branch if condition is 0
- return 42 in the then-branch → mov eax, 42; mov edi, eax; mov eax, 60; syscall
- Labels .L_else_0 / .L_end_0 provide the jump targets

Stage 4 implementation is complete. All features from the spec are implemented and all 28 tests pass. Would you like me to do anything else — for example, inspect the generated   
assembly for a specific test, or commit these changes?

✻ Cogitated for 1m 23s              