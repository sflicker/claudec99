# stage-83 Transcript: Project Source Converted to Strict ISO C99

## Summary

Stage 83 converts the ClaudeC99 compiler's own implementation source to strict ISO C99, ensuring it compiles cleanly under both `gcc -std=c99 -pedantic-errors` and `clang -std=c99 -pedantic-errors` with no GNU extensions or non-standard library functions. The change is purely conformance-oriented: no compiler tokenizer, parser, AST, or code generation behavior changes. All four root causes of C99 non-compliance are addressed: replacing the POSIX `strdup` function with a portable `util_strdup` helper, removing a redundant typedef, replacing GNU `__attribute__` directives with portable macros, and configuring CMake to enforce strict C99 at build time.

## Changes Made

### Step 1: Replace `strdup` with ISO C99-Compatible `util_strdup` Helper

- Added `char *util_strdup(const char *s);` declaration to `include/util.h`.
- Implemented `util_strdup()` in `src/util.c` using `malloc` and `memcpy`, with NULL-safety matching the behavior of libc's `strdup`.
- Added `#include "util.h"` to `src/lexer.c` (which was missing the include).
- Added `#include "util.h"` to `src/preprocessor.c` (which was missing the include).
- Replaced 1 call to `strdup()` in `src/lexer.c` with `util_strdup()`.
- Replaced 6 calls to `strdup()` in `src/preprocessor.c` with `util_strdup()`.

### Step 2: Remove Redundant ASTNode Typedef from codegen.h

- Deleted the redundant `typedef struct ASTNode ASTNode;` at line 84 of `include/codegen.h`. The type is already fully defined by the `#include "ast.h"` directive at the top of the file, making the forward typedef a C99 illegal redefinition.

### Step 3: Replace GNU `__attribute__` with Portable Macros

- Added macro definitions at the top of `include/util.h`:
  ```c
  #if defined(__GNUC__)
  #  define CC_NORETURN      __attribute__((noreturn))
  #  define CC_PRINTF(a, b)  __attribute__((format(printf, a, b)))
  #else
  #  define CC_NORETURN
  #  define CC_PRINTF(a, b)
  #endif
  ```
- Replaced 3 occurrences of `__attribute__((noreturn))` and `__attribute__((format(...)))` in `include/util.h` with the portable macros.
- Replaced 3 occurrences of `__attribute__((noreturn))` and `__attribute__((format(...)))` in `src/util.c` with the portable macros.

### Step 4: Lock CMakeLists.txt to Strict C99

- Changed `CMAKE_C_STANDARD` from `11` to `99`.
- Kept `CMAKE_C_STANDARD_REQUIRED ON`.
- Added `set(CMAKE_C_EXTENSIONS OFF)` to force `-std=c99` instead of `-std=gnu99`, enforcing strict ISO C99 mode.
- Added `target_compile_options(ccompiler PRIVATE -Wall -Wextra -pedantic)` to catch future regressions with enhanced diagnostics.

### Step 5: Update Version

- Updated `VERSION_STAGE` in `src/version.c` from `"00820500"` to `"00830000"`.

## Final Results

**Per-module strict C99 gate**: Compiled every source file with `gcc -std=c99 -pedantic-errors -Iinclude` and `clang -std=c99 -pedantic-errors -Iinclude` — zero errors from both compilers. PASS.

**Full build**: `cmake -S . -B build && cmake --build build` — BUILD OK.

**Test suite**: All 1259 tests pass (787 valid, 235 invalid, 73 integration, 43 print-AST, 100 print-tokens, 21 print-asm). No regressions; test counts unchanged from stage 82-05 (no new tests added — this was a behavior-preserving refactor).

## Session Flow

1. Read stage-83 specification and identified four root causes of C99 non-compliance.
2. Examined all src/*.c files to locate strdup call sites and attribute usages.
3. Designed and implemented util_strdup helper in util module, ensuring NULL-safety.
4. Updated lexer.c and preprocessor.c to include util.h and route strdup calls through util_strdup.
5. Removed redundant ASTNode typedef from codegen.h.
6. Added portable CC_NORETURN and CC_PRINTF macros to util.h and replaced all uses.
7. Configured CMakeLists.txt for strict C99: CMAKE_C_STANDARD 99, CMAKE_C_EXTENSIONS OFF, and warning flags.
8. Updated version identifier to stage-83.
9. Ran per-module gcc/clang strict C99 verification gate.
10. Built and ran full test suite; all 1259 tests pass with no regressions.
