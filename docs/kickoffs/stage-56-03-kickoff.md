# Stage 56-03: Command-Line Include Paths

## Spec Summary

Add support for `-I<dir>` and `-I <dir>` command-line flags to specify include search paths for quoted `#include "file.h"` directives.

### Search Order for Quoted Includes
1. Directory of the including file (existing behavior)
2. `-I` directories in command-line order (new)

### Out of Scope
- Angle includes (`#include <stdio.h>`)
- System include directories
- Environment include paths
- `-isystem` flag
- Header search diagnostics beyond "include file not found" message

## Tokenizer Changes
None required.

## Parser Changes
None required. Command-line argument parsing handled in `src/compiler.c`.

## AST Changes
None required.

## Code Generation Changes
None required.

## Test Plan

Create integration test `test/integration/test_cmdline_include_path/` with:
- Multiple subdirectories containing header files (`include/math.h`, `settings/include/settings.h`)
- `.cflags` file with multiple `-I` paths and `-D` macro definitions
- Main test file that includes headers from multiple search paths
- Expected return value: 7 (result of `add(A, B)` with A=3, B=4 from macro definitions)

## Planned Changes

### 1. `include/preprocessor.h`
Add public function declaration:
```c
PreprocessResult preprocess_with_defines_and_includes(
    const char *input_file,
    const char **defines,
    size_t n_defines,
    const char **include_dirs,
    size_t n_include_dirs
);
```

### 2. `src/preprocessor.c`
- Add `resolve_include_path()` helper function to search for quoted includes in include directories
- Thread `include_dirs` and `n_include_dirs` parameters through:
  - `preprocess_internal()`
  - `preprocess_file()`
- Implement new `preprocess_with_defines_and_includes()` public function
- Update `preprocess_with_defines()` to delegate to new function with empty include_dirs

### 3. `src/compiler.c`
- Parse `-I` flag in both `-Ipath` and `-I path` forms
- Collect include directories into array
- Call `preprocess_with_defines_and_includes()` instead of `preprocess_with_defines()`
- Update usage string to document `-I` flag

### 4. `test/integration/run_tests.sh`
- Change working directory to test subdirectory before running compiler
- Ensures relative `-I` paths in `.cflags` resolve correctly against test directory

### 5. `test/integration/test_cmdline_include_path/`
New integration test with:
- `include/math.h` — header with `add()` function declaration
- `settings/include/settings.h` — header with `FUNC` macro definition
- `.cflags` — `-Iinclude -Isettings/include -DA=3 -DB=4`
- `main.c` — includes both headers, uses macros in conditional compilation

## Implementation Order
1. Update `include/preprocessor.h` with new function declaration
2. Implement `resolve_include_path()` and thread include_dirs through preprocessor
3. Implement public `preprocess_with_defines_and_includes()` function
4. Update `src/compiler.c` to parse `-I` flags and call new function
5. Update test runner script to change directory appropriately
6. Create new integration test

## Ambiguities & Notes
- Spec contains minor typo: "ccompiler is involved" should be "invoked" (line 17) — not a blocker
- No grammar changes required (this is a preprocessor/command-line feature)
- Test runner must change into test directory so relative paths work correctly
- Both `-Ipath` (concatenated) and `-I path` (separate arg) forms must be supported
