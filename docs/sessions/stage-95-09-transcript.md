# stage-95-09 Transcript: Dynamic String Storage for AST Values

## Summary

Stage 95-09 completes the removal of fixed-size `char value[MAX_NAME_LEN]` buffers from the AST layer by replacing them with `const char *value` pointers into lexer-owned storage. This eliminates buffer overflow risk for long identifiers, string literals, and numeric constants. The stage involved updating the AST node structure, lexer API exposure, parser handling of all value-bearing constructs, and safety checks in codegen and the pretty printer. A critical fix for case label formatting resolved a latent use-after-stack memory bug that was causing all 17 switch statement tests to fail.

## Changes Made

### Step 1: AST Structure

- Changed `ASTNode.value` from `char value[MAX_NAME_LEN]` to `const char *value` in `include/ast.h`.
- Updated `ast_new()` in `src/ast.c` to assign the pointer directly (`node->value = value`) instead of using `strncpy()`.

### Step 2: Lexer Exposure

- Removed `static` qualifier from `lexer_store_bytes()` in `src/lexer.c`.
- Exposed `lexer_store_bytes()` declaration in `include/lexer.h` to allow parser access.

### Step 3: Parser Refactoring

- Added `#include "strbuf.h"` to `src/parser.c` for string handling.
- Updated `ParsedDeclarator.name` from `char[256]` array to `const char *` pointer.
- Replaced two `strncpy(d.name, name.value, ...)` calls with direct assignment `d.name = name.value`.
- Refactored string literal handling:
  - Single string literals: replaced `memcpy` with `node->value = token.value`.
  - Concatenated string literals: accumulate in StrBuf scratch buffer, then `lexer_store_bytes()` and assign.
  - Removed `MAX_NAME_LEN` length checks that were guarding against buffer overflow.
- Updated char literal handling: replaced `node->value[0] = token.value[0]; node->value[1] = '\0'` with `node->value = token.value`.
- Refactored `__builtin_va_arg` type name handling: replaced `snprintf` with `node->value = type_kind_name(...)` to use pre-existing type name storage.
- Refactored enum constant formatting: replaced inline `snprintf` into fixed buffer with `lexer_store_bytes()` call after formatting into local `char[32]`.
- Fixed case label formatting bug: replaced `snprintf` into local `char[32]` buffer passed to `parser_node()` with proper `lexer_store_bytes()` call. (This was a latent use-after-stack bug causing all 17 switch tests to fail.)
- Updated local/file-scope char array initializers: replaced `memcpy + null-terminator` with direct assignment from `str_tok.value`.

### Step 4: Safety Checks

- Added NULL guard in `src/codegen.c` for `node->value[0] != '\0'` check in AST_ASSIGNMENT codegen path.
- Added NULL guard in `src/ast_pretty_printer.c` for `node->value[0] == '\0'` check in AST_ASSIGNMENT pretty-print path.

### Step 5: Version and Documentation

- Updated `VERSION_STAGE` in `src/version.c` to `00950900`.
- Updated `docs/fixed-capacity-inventory.md` to mark `ASTNode.value` as complete in the `MAX_NAME_LEN` removal row.
- Updated `README.md` with stage description and final test totals.

### Step 6: Test Coverage

- Added `test/valid/test_stage_95_09_long_string_literal__42.c` with a 260-character string literal to verify the new unlimited string storage capacity (exceeds old 255-byte limit).

## Final Results

All 1479 tests pass at C0, C1, and C2:
- 165 unit tests pass
- 828 valid C code tests pass
- 237 invalid C code tests pass
- 82 integration tests pass
- 43 print_ast tests pass
- 100 print_tokens tests pass
- 21 print_asm tests pass

Self-host bootstrapping succeeded: C0→C1→C2 with no failures.

No regressions.

## Session Flow

1. Reviewed stage 95-09 specification and implementation summary.
2. Examined AST node structure and lexer storage API to understand current design.
3. Traced parser code paths for string literals, char literals, enum constants, and case labels.
4. Identified the latent case label bug (use-after-stack via local `char[32]` buffer).
5. Implemented AST pointer change and lexer API exposure.
6. Updated parser to use lexer storage for all value-bearing constructs.
7. Added safety checks in codegen and pretty printer.
8. Verified all 1479 tests pass at C0, C1, C2 and self-hosting succeeds.
9. Generated milestone summary and transcript artifacts.

## Notes

**Unnamed Function Parameters**: Retained `""` (static empty string) as default value rather than NULL to maintain compatibility with callers that use `strcmp()` and expect non-NULL pointers (e.g., `codegen_add_var`).

**Operator Strings**: Static C string literals like `"+"`, `"-"` have permanent storage duration and are safely used as `const char *` without lexer allocation.

**Case Label Bug**: The original parser implementation formatted case label values into a local `char[32]` stack buffer and passed it to `parser_node()`, which stored a pointer into that buffer in the AST. This created a dangling pointer bug causing crashes or undefined behavior after the stack frame was deallocated. The fix ensures all formatted values are stored in lexer-owned storage via `lexer_store_bytes()`. This fix unblocked all 17 switch statement test cases.
