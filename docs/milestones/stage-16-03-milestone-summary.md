  Milestone Summary                                                                                                                                                                               
   
  stage-16-03 ships the binary shift operators << and >> end-to-end:                                                                                                                              
                                                                                                                                                                                                
  - Tokenizer: TOKEN_LEFT_SHIFT / TOKEN_RIGHT_SHIFT, matched before < / > / <= / >=.                                                                                                              
  - Grammar/Parser: new <shift_expression> between <relational_expression> and <additive_expression>; new parse_shift, reusing AST_BINARY_OP. Shift binds less tightly than +/- and more tightly
  than </>.                                                                                                                                                                                       
  - Codegen: result type follows the promoted left operand. shl/sar against eax (int) or rax (long), with the count's low byte in cl. Pointer/array operands rejected on either side.           
  - Tests: 28 new (15 valid, 8 invalid, 2 print-tokens, 3 print-asm). Full suite 505/505 pass.                                                                                                    
  - Docs: docs/grammar.md updated; README.md bumped to "Through stage 16-03" with refreshed totals.                                                                                               
  - Spec issue called out: the spec's long-right-shift test used >>> (not a C operator); treated as a typo for >>.                                                                                
                                                                                                                                                                                                  
  Artifacts saved at docs/kickoffs/stage-16-03-kickoff.md, docs/milestones/stage-16-03-milestone.md, and docs/sessions/stage-16-03-transcript.md.          

