  Milestone Summary                                          
                                                          
  Milestone Summary                        
                                                
  Stage-16-04: Bitwise binary operators (&, ^, |) — Complete
                                                          
  stage-16-04 ships the binary bitwise operators & (AND), ^ (XOR), and | (OR) on integer operands.                                                                                                                              
                                                          
  - Tokenizer: introduced TOKEN_CARET (^) and TOKEN_PIPE (|); bare | previously fell through to TOKEN_UNKNOWN and || still wins. & continues to use TOKEN_AMPERSAND; the parser disambiguates the binary form from unary        
  address-of by position. --print-tokens learned both new names.                                                                                                                                                              
  - Grammar/Parser: inserted three new precedence levels between <equality_expression> and <logical_and_expression> — <bitwise_and_expression> (tightest), <bitwise_xor_expression>, <bitwise_or_expression>.                   
  <logical_and_expression> now consumes <bitwise_or_expression>. New parsers are left-associative; reuse AST_BINARY_OP with value "&" / "^" / "|".                                                                              
  - AST/Semantics: no new node types. Pretty printer learns BITAND / BITXOR / BITOR for symmetry with AND / OR.                                                                                                                 
  - Codegen: expr_result_type returns common_arith_kind(left, right) for the three operators (char/short/int → int; either side long → long). Emission reuses the arithmetic-binary-op pattern (left → push → right → pop into  
  rcx, plus movsxd widening when common == long), then and|xor|or eax, ecx or and|xor|or rax, rcx. Pointer/array operands rejected with the existing not supported on pointer operands diagnostic.                              
  - Tests: 30 new (13 valid, 11 invalid, 2 print-tokens, 4 print-asm). Full suite 535/535 pass (321 valid + 91 invalid + 24 print-AST + 80 print-tokens + 19 print-asm). No regressions.                                        
  - Docs: docs/grammar.md adds the three new productions and rewrites <logical_and_expression>; README.md bumped to "Through stage 16-04" with a new bitwise-binary bullet and refreshed test totals.                           
  - Notes: Spec issues called out and resolved during kickoff — reutrn typo (treated as return), the literal ^&a in the first invalid example (rewritten to &a to cleanly cover the pointer-operand rejection), and the missing 
  result-type rule (adopted the standard common-arithmetic-type rule that the rest of the binary arithmetic operators use).                                                                                                     
                                                                                                                                                                                                                                
  Transcript                                                                                                                                                                                                                    
                                                                                                                                                                                                                              
  Saved to docs/sessions/stage-16-04-transcript.md per the format template — Summary, Plan, Changes Made (Steps 1–6: Tokenizer / Parser / AST / Code Generator / Tests / Documentation), Final Results, Session Flow, and Notes.
   
  Generated Artifacts                                                                                                                                                                                                           
                                                                                                                                                                                                                              
  - docs/kickoffs/stage-16-04-kickoff.md                                                                                                                                                                                        
  - docs/milestones/stage-16-04-milestone.md
  - docs/sessions/stage-16-04-transcript.md                                                                                                                                                                                     
  - 13 valid tests, 11 invalid tests, 2 print-tokens tests, 4 print-asm tests                                                                                                                                                 
  - Source changes: include/token.h, src/lexer.c, src/compiler.c, src/parser.c, src/ast_pretty_printer.c, src/codegen.c                                                                                                         
  - Doc updates: docs/grammar.md, README.md                                                                                                                                                                                     
  - Commit: 2f5c800 — stage-16-04: bitwise binary operators (&, ^, |)                                                                                                                                                           
                                                                                  
