  Milestone Summary
                                                                                                                                                              
  Stage-14-06: Char Array Initialization from String Literals — Complete                                                                                      
                                                                                                                                                              
  - Parser (src/parser.c): array-suffix branch now accepts an empty [], accepts a string-literal initializer for char arrays (with element-type and capacity  
  validation), and infers the length from the literal when the size is omitted. Emits the four spec-listed errors.                                          
  - Codegen (src/codegen.c): AST_DECLARATION/TYPE_ARRAY reads the optional AST_STRING_LITERAL child and emits per-byte mov byte [rbp - K], <byte> stores      
  covering payload + NUL + zero fill.                                                                                                                         
  - Tests: 6 new valid, 4 new invalid, 1 new print-AST snapshot, 2 new print-asm snapshots.                                                                 
  - Docs: docs/grammar.md updated; README.md "Through stage", arrays bullet, and aggregate counts refreshed.                                                  
  - Result: 402/402 tests pass (250 valid + 49 invalid + 23 print-AST + 73 print-tokens + 7 print-asm). No regressions.                                       
                                                                                                                                                              
  Artifacts saved:                                                                                                                                            
  - docs/kickoffs/stage-14-06-kickoff.md                                                                                                                      
  - docs/milestones/stage-14-06-summary.md                                                                                                                    
  - docs/sessions/stage-14-06-transcript.md          

