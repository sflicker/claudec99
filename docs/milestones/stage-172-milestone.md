# Milestone Summary

## Stage 172: Build Tool Compatibility (make and cmake) - Complete

Stage 172 ships build tool compatibility support by accepting a comprehensive set of compiler and linker flags commonly emitted by GNU make and CMake. This is a pure CLI-only stage with no tokenizer, parser, AST, or codegen changes.

- Tokenizer: no changes.
- Grammar/Parser: no changes.
- AST/Semantics: no changes.
- Codegen: no changes.
- Compiler (`ccompiler`): Added `-std=c99/-std=gnu99/-std=c11/-std=gnu11/-std=c17/-std=gnu17/-std=c2x/-ansi` branch (accepted, ignored—always target C99); added `-isystem <dir>` branch (treated as `-I<dir>`); added `-w` branch (accepted, no-op until warning diagnostics land); updated `--help` output.
- Wrapper script (`bin/cc99`): Fixed `-O2` forwarding (was falling through to "unrecognized option"); added `-O3/-Os/-Og/-Ofast` (silently dropped; our max is -O2); added `-std=*/-ansi` forwarding to ccompiler; added `-w` forwarding to ccompiler; added `-Wno-*` silent discard; added PIC/PIE discard flags (`-fPIC/-fPIE/-fpic/-fpie/-fno-pie/-fno-PIC/-fno-pic/-fno-PIE`); added hardening/codegen flags discard (`-fstack-protector/-fstack-protector-all/-fstack-protector-strong/-fno-stack-protector`, `-fvisibility=*`, `-fomit-frame-pointer/-fno-omit-frame-pointer`, `-fstrict-aliasing/-fno-strict-aliasing`, `-ffunction-sections/-fdata-sections`, `-pipe`); added machine-tuning discard (`-march=*/-mtune=*/-m64/-m32`); added `-isystem <dir>` forwarding to ccompiler; added dependency-tracking stubs (`-MD/-MP` discard, `-MF/-MT/-MQ <arg>` discard with arg); added `-pthread` (appends `-lpthread` to link_flags); updated `--help` output.
- Tests: New `test/build_tool/` suite with 3 tests (test_make_hello, test_make_cflags, test_cmake_hello; cmake test auto-skipped if cmake not installed). Full suite: 2072 portable (165 unit, 1286 valid, 261 invalid, 189 integration, 50 print-AST, 100 print-tokens, 21 print-asm) + 189 system-include + 2 optional-library + 3 build-tool pass.
- Docs: Updated `docs/outlines/checklist.md` to mark Stage 172 complete; updated `README.md` to reflect Stage 172 as current stage and document new build-compat flags.
- Notes: All new feature acceptance is transparent—flags are parsed but ignored or minimally acted upon (e.g., `-pthread` appends a link flag, `-isystem` forwards as `-I`). This enables drop-in compatibility with standard build tools without implementing the full semantics (e.g., stack-protector hardening is not actually performed). Self-host C0→C1→C2 verified with all tests passing at every stage.
