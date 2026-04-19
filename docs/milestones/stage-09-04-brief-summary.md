● Milestone Summary — stage-09-04

Stage 9.4 is complete. The compiler now supports function call expressions with parser-side validation (undefined callee, arg-count mismatch, >6 args, forward references), SysV AMD64 register passing (rdi, rsi, rdx, rcx, r8, r9), and self-recursion. The code generator tracks     
push_depth to keep rsp 16-aligned at each call, and the non-main return epilogue (mov rsp, rbp; pop rbp; ret) is now correctly emitted — fixing a latent stage-9.3 bug that only becomes observable when real calls happen. The valid-test linker invocation now sets -e main so a      
callee defined before main does not become the default entry.

Test results: 103/103 pass (86 valid + 9 invalid + 8 print_ast). No regressions.

Artifacts:
- docs/kickoffs/stage-09-04-kickoff.md
- docs/milestones/stage-09-04-summary.md
- docs/sessions/stage-09-04-transcript.md
- docs/grammar.md synced
- Commit bdbd7a4      
