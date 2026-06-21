# stage-163 Transcript: Non-Constant Initializer for Global

## Summary

Stage 163 fixes a critical compatibility bug where global pointer declarations initialized with `NULL` were rejected as having a "non-constant initializer." The bug occurred when `NULL` was defined (as in GCC system headers) as `((void *)0)` — a null pointer constant cast. The parser's validator checked for accepted AST node types but did not recognize `AST_CAST` nodes as valid global pointer initializers. The fix extends the validator to accept null pointer constant casts (`AST_CAST` from `TYPE_POINTER` containing `AST_INT_LITERAL("0")`) and adds corresponding codegen support to emit `dq 0` for such globals.

## Changes Made

### Step 1: Parser Validation

- Located pointer-global initializer validator in `src/parser.c` (~line 4639).
- Existing validator checked for: `AST_INT_LITERAL`, `AST_CHAR_LITERAL`, `AST_STRING_LITERAL`, `AST_VAR_REF`, `AST_ADDR_OF`, `AST_COMPOUND_LITERAL`.
- Added new condition: accept `AST_CAST` nodes where `node->decl_type == TYPE_POINTER` and the child is `AST_INT_LITERAL("0")`.
- This pattern precisely matches `((void *)0)` after the parser builds the cast node.

### Step 2: Code Generator Support

- Added new branch in `codegen_add_global` (in `src/codegen.c`) to handle `TYPE_POINTER` globals with `AST_CAST` initializers.
- When the initializer is a null pointer constant cast, sets `init_value = 0`, `is_initialized = 1`.
- Emits `dq 0` in the data section (8-byte zero pointer).

### Step 3: Version Update

- Updated `src/version.c` to bump `VERSION_STAGE` to "01630000".

### Step 4: Integration Test

- Created `test/integration/test_null_cast_global/test_null_cast_global.c` testing both `Point *gp = NULL;` and `int *gi = NULL;` where `NULL` is `((void *)0)`.
- Verifies both pointers are null (via address comparison and dereferenced checks).
- Added corresponding `.expected` output file.

## Final Results

All 2066 portable tests pass (165 unit, 1286 valid, 261 invalid, 183 integration, 50 print-AST, 100 print-tokens, 21 print-asm). System-include: 183 pass. Optional-library: 2 pass (test_sdl2_init, test_zlib_compress). Self-host C0→C1→C2 verified with all tests passing at every stage; no source changes needed during bootstrap.

## Session Flow

1. Read spec and identified the root cause: `NULL` as `((void *)0)` produces `AST_CAST` which was not in the accepted initializer list.
2. Reviewed parser validator in `src/parser.c` to understand the existing check.
3. Extended the validator to recognize null pointer constant casts.
4. Implemented codegen path in `src/codegen.c` to emit `dq 0` for such globals.
5. Updated version in `src/version.c`.
6. Created integration test with NULL-cast globals.
7. Built and verified all 2066 portable tests pass.
8. Self-host C0→C1→C2 cycle verified with no source changes needed.
