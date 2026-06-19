# Milestone Summary

## Stage 141: System Includes - Complete

Stage 141 ships a `--sysinclude` flag and integrated test infrastructure for validating compiler behavior against real system headers.

- **Tooling**: `bin/cc99` adds `--sysinclude` flag (Linux x86_64 only) to replace stub `test/include` headers with real system paths (`/usr/lib/gcc/x86_64-linux-gnu/13/include`, `/usr/local/include`, `/usr/include/x86_64-linux-gnu`, `/usr/include`).
- **Testing**: `test/run_all_tests.sh` detects Linux x86_64 and runs `test/integration/run_tests_sysinclude.sh` as a separate suite; results reported in "System include:" section distinct from portable "Portable:" aggregate.
- **Compiler**: No language features or code generation changes; version bumped to 14100000.
- **Results**: **1982 portable tests pass** (1284 valid, 262 invalid, 98 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit); **98/99 system-include tests pass** (1 pre-existing `bits/wchar.h` L'\0' wide-char-literal limitation).
- **Self-host**: C0→C1→C2 verified with all 1982 portable tests passing at each stage.
