# Stage 56-04: Angle-Bracket Includes

## Spec Summary

Add support for `#include <filename>` (angle-bracket includes) that search only in `-I` directories without checking the directory of the including file first. This complements stage 56-03's quoted include support.

### Search Order
- **Quoted includes** (`#include "file"`): Directory of including file first, then `-I` directories in order
- **Angle includes** (`#include <file>`): `-I` directories in order only (no directory-of-including-file step)

### Out of Scope
- Host system include directories (e.g., `/usr/include`)
- Compiler default include directories
- `-isystem` flag
- Standard header compatibility
- `#include_next`

## Tokenizer Changes

None required. Lexer already handles `<` and `>` tokens; we parse them at the preprocessor level.

## Parser Changes

None required. Include directive handling is in the preprocessor.

## AST Changes

None required.

## Code Generation Changes

None required.

## Test Plan

Create integration test `test/integration/test_pp_angle_include/` with:
- `include/add.h` — function declaration `int add(int a, int b);`
- `add.c` — implements `add()` and includes `<add.h>`
- `test_pp_angle_include.c` — calls `add(3,4)`, includes `<add.h>`
- `test_pp_angle_include.cflags` — contains `-Iinclude`
- Expected return value: 7

## Planned Changes

### 1. `include/preprocessor.h`
Update docstring for `preprocess_with_defines_and_includes()` to document angle-bracket include support and clarify search order difference from quoted includes.

### 2. `src/preprocessor.c`
- Add `resolve_angle_include_path()` function:
  - Searches only in `include_dirs[]` in command-line order
  - No directory-of-including-file step (unlike quoted includes)
  - Returns full path to found file or NULL if not found
- Update `#include` directive handler to:
  - Parse both `<filename>` and `"filename"` forms
  - Dispatch to `resolve_angle_include_path()` for angle includes
  - Dispatch to existing `resolve_include_path()` for quoted includes

### 3. `test/integration/test_pp_angle_include/`
New integration test directory with required files:
- `include/add.h`
- `add.c`
- `test_pp_angle_include.c`
- `test_pp_angle_include.cflags`
- `test_pp_angle_include.status` (expected exit code: 7)

## Implementation Order

1. Implement `resolve_angle_include_path()` in `src/preprocessor.c`
2. Update `#include` directive handler in `src/preprocessor.c` to parse and dispatch angle includes
3. Update docstring in `include/preprocessor.h`
4. Create integration test in `test/integration/test_pp_angle_include/`
5. Verify test passes

## Ambiguities & Notes

- Spec is clear: angle includes search `-I` directories only, no source-file directory step
- Parser already distinguishes `<` from `"` in preprocessing context, so both forms can coexist
- Integration test structure mirrors stage 56-03 pattern with `.cflags` and `.status` companion files
- This stage is directly dependent on stage 56-03 (requires `-I` flag support already implemented)
