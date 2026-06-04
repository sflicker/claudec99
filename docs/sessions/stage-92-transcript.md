# stage-92 Transcript: Self-Compile Validation

## Summary

Stage 92 pursued full self-compilation by using the gcc-built compiler (C0) to build itself (C1), running the full test suite against C1, and then (if successful) building C2 from C1 and testing again. The bootstrap succeeded in revealing real bugs and capacity limits but halted at a struct-by-value limitation in the lexer interface—a documented unsupported feature. Seven defects/limits were identified and fixed (including a critical silent AST-truncation bug), all tests remain green at 1302/1302, and the remaining blocker was documented for a future dedicated stage. Per the user's decision, the MINOR version was deliberately NOT bumped (stays "01"), and no self-compilation claim is made.

## Plan

The stage plan was:
1. Read the spec and understand the bootstrap workflow.
2. Build C0 (gcc-built compiler, existing).
3. Run C0 on all compiler source files to generate `.asm`.
4. Assemble and link the generated code into C1.
5. Run all tests with C1 (expecting all pass or failure points).
6. Document failures and fix real blockers.
7. Iterate: rebuild and re-test after each fix.
8. If all tests pass with C1, bump MINOR, rebuild to C2 with C1, and test C2.
9. If C2 tests also pass, declare self-compilation achieved.
10. If blockers remain, document them and defer implementation to a future stage.

## Changes Made

### Step 1: AST Children Array — Dynamic Allocation

- Converted `ASTNode.children[AST_MAX_CHILDREN]` (fixed array, max 64) to a lazily-allocated, doubling dynamic array in `include/ast.h`.
- Added `size_t child_cap` field to track capacity; `int child_count` continues to track the current size.
- Updated `ast_add_child()` in `src/ast.c` to dynamically grow the array by doubling when full (starting at capacity `AST_MAX_CHILDREN`).
- Updated `ast_free()` to free the dynamically allocated children array.
- This fix addresses the critical silent truncation bug: translation units, large blocks, and switch statements with >64 top-level declarations/statements now preserve all children. Previously, declarations past the 64th were dropped with no diagnostic, including `main` in `compiler.c`.

### Step 2: Parser — For-Statement Children

- Updated the `for`-statement builder in `src/parser.c` to append its four child nodes via `ast_add_child()` instead of writing to fixed array slots.
- Required by the dynamic children array implementation; the parser must use the public interface for all child additions.

### Step 3: Code Generator — Extern Deduplication and Redefinition Fix

- Modified `codegen_emit_externs()` in `src/codegen.c` to suppress `extern` directives for any object that is **both** declared `extern` (via a header) and defined in the same translation unit.
- Added deduplication: repeated `extern` directives for the same symbol are emitted only once.
- This fixes NASM errors like `label 'g_error_src_line' inconsistently redefined` that arose when a file declares a global via its own header and also defines it.

### Step 4: Constants — Raise Limits

- Updated `include/constants.h`:
  - `MAX_STRING_LITERALS`: default 256 → 2048 (the compiler's codegen has ~750 literal occurrences and the pool does not dedupe).
  - `MAX_SWITCH_LABELS`: default 64 → 256 (the `token_type_name()` switch statement covers ~83 token kinds).

### Step 5: Code Generator — Hoist Static Arrays to File Scope

- The compiler does not yet support block-scope `static` arrays; six such tables existed in `src/codegen.c` (register name arrays).
- Hoisted these six arrays from block scope to file scope, retaining `static` linkage to limit visibility.
- Semantics unchanged; code generation behavior identical.

### Step 6: Stub Headers — Add Missing Declarations

- Updated `test/include/stdlib.h` to add declarations for `strtol()` and `strtoul()`, which are used in `src/compiler.c` for parsing compiler arguments.
- Previously these were undefined at compile time, causing the bootstrap to fail with "call to undefined function" errors.

### Step 7: Main Signature — Fix Parameter Type

- Changed the signature of `main()` in `src/compiler.c` from `int main(int argc, char *argv[])` to `int main(int argc, char **argv)`.
- The `T *name[]` parameter form is mis-typed in subscript operations (element type becomes integer instead of pointer); only the `T **name` form works correctly.
- This is semantically equivalent and matches the C standard convention.

### Step 8: Version

- Updated `VERSION_STAGE` in `src/version.c` from `"00910000"` to `"00920000"`.
- Intentionally kept `VERSION_MINOR` at `"01"` (did not bump to "02") because full self-compilation was not achieved.

## Final Results

**Compilation Status:** Seven fixes enabled modules `ast.c`, `ast_pretty_printer.c`, `codegen.c`, `compiler.c`, `type.c`, `util.c`, and `version.c` to compile, assemble, and link successfully during bootstrap. The bootstrap halted at `lexer.c:116` with the error `'.' applied to non-struct/union 'token'` — the lexer/parser interface passes and returns `Token` by value, which the compiler does not yet support.

**Test Status:** All 1302 tests pass (819 valid, 237 invalid, 82 integration, 43 print_ast, 100 print_tokens, 21 print_asm). No regressions. The gcc-built C0 test suite remains fully green.

**Bootstrap Status:** NOT yet self-hosting. The bootstrap successfully surfaced real defects (silent AST truncation, extern redefinition, capacity limits, missing declarations, type mismatches) and fixed all seven. However, the struct-by-value limitation in the Token interface blocks further progress. Resolving this requires implementing SysV AMD64 struct-by-value parameters/returns or refactoring the lexer/parser interface — both are out of scope for a validation stage and are deferred to a dedicated future stage.

## Session Flow

1. Read the spec and supporting documentation.
2. Built the gcc-built compiler (C0).
3. Attempted bootstrap: ran C0 on all compiler source files and tried to assemble/link the result.
4. Encountered the first blocker: NASM redefinition errors for globals declared extern in headers and defined in the same module.
5. Implemented Fix #1 (extern deduplication) and re-tested.
6. Encountered the second blocker: link error "undefined reference to 'main'" and silent truncation of large files (discovered by inspecting the generated AST).
7. Implemented Fix #2 (dynamic AST children array) and Fix #3 (parser for-statement builder update), re-tested.
8. Encountered the third and fourth blockers: capacity exceeded for string literals and switch labels.
9. Implemented Fix #4 (raise limits in constants.h) and re-tested.
10. Encountered the fifth blocker: block-scope static arrays not yet supported.
11. Implemented Fix #5 (hoist arrays to file scope) and re-tested.
12. Encountered the sixth blocker: missing stdlib declarations for strtol/strtoul.
13. Implemented Fix #6 (add declarations to stub stdlib.h) and re-tested.
14. Encountered the seventh blocker: parameter mis-typing in main signature.
15. Implemented Fix #7 (change main signature to char **argv form) and re-tested.
16. Bootstrap advanced to lexer.c but halted due to struct-by-value limitation (a known unsupported feature).
17. Documented the findings in `docs/self-compilation-report.md`.
18. Committed all seven fixes and documented that full self-hosting is deferred.
19. Updated version stage (not minor) and confirmed all tests remain green.
20. Generated milestone summary and transcript.

## Notes

- **Silent AST Truncation:** The most serious bug fixed was the silent truncation of large translation units caused by `ast_add_child()` dropping children past the hard-coded limit of 64. This invalidated the prior self-compilation report, which only checked `.asm` generation (not assembly/linking/testing) and therefore did not catch the truncation.

- **Bootstrap Methodology:** A true bootstrap requires compile → assemble → link → test, not just checking `.asm` generation. Per-module `.asm` checks miss truncation, linking errors, and undefined symbols.

- **Struct-by-Value Limitation:** The remaining blocker is the Token interface, which passes and returns the Token struct by value. Supporting this requires either (a) implementing SysV AMD64 memory-class struct parameters/returns (via hidden pointer in rdi) or (b) refactoring the lexer/parser to use pointers/out-parameters. Both are substantial and are deferred to a dedicated future stage.

- **Version Bump Decision:** The MINOR version was intentionally NOT bumped from "01" to "02" because full self-compilation was not achieved. The project is still gcc-only; this stage is a validation attempt that documented the path forward, not a fully successful milestone.

- **Regression Testing:** All 1302 existing tests pass with the gcc-built compiler after the seven fixes. No tests were added this stage because the focus was on compiler self-compilation, not new language features.

- **Additional Blockers Possible:** The bootstrap only reached lexer.c; modules parser.c and preprocessor.c have not yet been validated end-to-end. Additional blockers may exist beyond the struct-by-value gap.
