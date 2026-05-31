# Stage 83 Kickoff

## Summary of the Spec

Stage 83 converts the ClaudeC99 compiler's own source code to conform to strict ISO C99. The goal is to compile clean under both `gcc -std=c99 -pedantic-errors` and `clang -std=c99 -pedantic-errors` with no GNU extensions or non-standard library functions.

The stage addresses **three source root causes plus a build-config change**:

1. **`strdup` is POSIX, not ISO C99** — 7 call sites (1 in `src/lexer.c`, 6 in `src/preprocessor.c`) cause implicit-declaration and pointer-from-int errors. Solution: add a `util_strdup` helper function (malloc + memcpy) to the util module and route all calls through it. Avoid naming it `strdup` to dodge glibc redeclaration conflicts.

2. **Redundant `typedef struct ASTNode ASTNode;`** at `include/codegen.h:84` — illegal in C99 (typedef redefinition is a C11 feature) and unnecessary since `codegen.h` already `#include "ast.h"` which fully defines `ASTNode`. Solution: delete it.

3. **GNU extension attributes** (`__attribute__((noreturn))` / `__attribute__((format(printf,...)))`) — 3 sites in `include/util.h`, 3 in `src/util.c`. Solution: wrap in `CC_NORETURN` / `CC_PRINTF(a,b)` macros guarded by `#if defined(__GNUC__)`.

4. **CMakeLists.txt** — set `CMAKE_C_STANDARD 99`, keep `CMAKE_C_STANDARD_REQUIRED ON`, add `CMAKE_C_EXTENSIONS OFF` (forces `-std=c99` not `-std=gnu99`), and add `target_compile_options(ccompiler PRIVATE -Wall -Wextra -pedantic)`.

## Required Tokenizer Changes

None. This stage makes no lexer behavior change. `src/lexer.c` is edited only to add `#include "util.h"` and swap `strdup` → `util_strdup`.

## Required Parser Changes

None.

## Required AST Changes

None.

## Required Code Generation Changes

None. `include/codegen.h` is edited only to remove a redundant typedef line; behavior-preserving.

## Test Plan

1. **Strict per-module gate** (acceptance criterion): For each `src/*.c`, run both compilers with `-std=c99 -pedantic-errors -Iinclude -DVERSION_BUILD=0 -fsyntax-only`. Expect zero errors from both.

2. **Full build**: `cmake -S . -B build && cmake --build build` must succeed.

3. **Behavior unchanged**: `./test/run_all_tests.sh` — all previously-passing tests must still pass. No new test files are added by this stage (it is a source-conformance/build change, not a language feature).

## Critical Files

- `include/util.h` — add `util_strdup` declaration, add `CC_NORETURN` / `CC_PRINTF` macros, replace attributes
- `src/util.c` — implement `util_strdup`, replace attributes
- `src/lexer.c`, `src/preprocessor.c` — swap `strdup` → `util_strdup`, add `#include "util.h"`
- `include/codegen.h` — delete redundant `ASTNode` typedef
- `CMakeLists.txt` — C99 + extensions off + warning flags

## Implementation Order

1. Add `util_strdup` declaration to `include/util.h`, implement in `src/util.c`
2. Add `CC_NORETURN` / `CC_PRINTF` macros to `include/util.h`
3. Replace all 6 attribute uses (3 in `util.h`, 3 in `src/util.c`) with the macros
4. Update `src/lexer.c` to include `util.h` and replace `strdup` with `util_strdup`
5. Update `src/preprocessor.c` to include `util.h` and replace 6 `strdup` calls with `util_strdup`
6. Remove redundant `ASTNode` typedef from `include/codegen.h:84`
7. Update `CMakeLists.txt`: set C99, disable extensions, add warning flags
8. Run strict per-module test gate, full build, then test suite
