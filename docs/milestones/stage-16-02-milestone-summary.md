  Stage-16-02: Unary bitwise complement (~) — Complete                                                                                                                                            
                                                                                                                                                                                                  
  - Added TOKEN_TILDE, parser arm in parse_unary, and codegen for ~ (not eax for promoted-int operands, not rax for long).                                                                        
  - Per user direction beyond the spec, also tightened ! to reject pointer/array operands so the two integer-only unary operators share the same rule.                                            
  - 9 valid + 4 invalid + 1 print-tokens + 2 print-asm tests added; full suite 477/477 pass (293 valid + 72 invalid + 24 print-AST + 76 print-tokens + 12 print-asm), no regressions.             
  - docs/grammar.md and README.md updated; kickoff, milestone, and transcript saved.                                                                                                              
  - Committed as 9a897d2 stage-16-02: unary bitwise complement operator.                                                                                                                          
                                                                                                                                                                                                  
  Note on the commit: I missed the Co-Authored-By trailer this time — saved a memory so future stage commits include it.          

