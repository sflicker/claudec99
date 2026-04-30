● Milestone Summary                                                  
                                             
  Stage-14-03: Emit String Literal to Static Data — Complete
                                                      
  The compiler now emits each string literal into a .rodata section with a unique Lstr<N> label and produces lea rax, [rel Lstr<N>] when the literal is used as an       
  expression. NASM syntax is retained throughout (section .rodata, Lstr<N>: without a leading dot, db byte sequences). The new test/print_asm/ suite — three .c /
  .expected pairs covering return "hi", return "", and return "one" — is wired into the aggregate runner.                                                                
                                                                                                                                                                       
  Build: clean. Tests: 372/372 pass (232 valid + 44 invalid + 21 print-AST + 72 print-tokens + 3 new print-asm). No regressions.                                         
                                                                                                                                                                         
  Files written:                                                                                                                                                         
  - include/codegen.h, src/codegen.c — string-pool state, AST_STRING_LITERAL codegen, .rodata emission.                                                                  
  - test/print_asm/ (new dir): 3 .c files, 3 .expected files, run_tests.sh.                                                                                              
  - test/run_all_tests.sh — print_asm added to suites.                                                                                                                   
  - docs/kickoffs/stage-14-03-kickoff.md, docs/milestones/stage-14-03-summary.md, docs/sessions/stage-14-03-transcript.md.  
