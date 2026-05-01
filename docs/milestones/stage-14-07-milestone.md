  Milestone Summary                                                                                                                                           
                                                                                                                                                              
  Stage-14-07: Calling libc puts — Complete                                                                                                                   
                                                                                                                                                              
  - Codegen (src/codegen.c):                                                                                                                                  
    - New function_has_body, tu_has_definition_for, codegen_emit_externs helpers; extern <name> emitted for each declared-but-not-defined function in the TU.
    - Function prologue (push rbp; mov rbp, rsp) now always emitted — under gcc -no-pie linking, _start calls main, so rsp is 8 mod 16 at entry; the          
  unconditional push restores 16-byte alignment for SysV AMD64 internal calls (libc routines using SSE).                                                      
    - main's exit path changed from mov eax, 60; syscall to a normal ret so __libc_start_main calls libc exit (which runs stdio cleanup — required for        
  buffered puts output to flush to non-TTY stdouts).                                                                                                          
  - Test runner (test/valid/run_tests.sh): switched from ld -e main to gcc -no-pie; binary stdout redirected to /dev/null during exit-code checks.          
  - Tests: 2 new valid runtime tests (puts hello, two puts calls), 1 new print_asm snapshot, 7 prior print_asm snapshots refreshed for the new prologue/exit  
  shape.                                                                                                                                                      
  - Docs: README updated (through-stage line, functions bullet, end-to-end example, test counts).                                                             
  - Result: 405/405 tests pass (252 valid + 49 invalid + 23 print-AST + 73 print-tokens + 8 print-asm). No regressions.        

