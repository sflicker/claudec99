# ClaudeC99 Stage 93 convert project to std c99

## Context

The project currently builds under `-std=gnu11` (CMake sets `CMAKE_C_STANDARD 11`,
extensions left on). The goal is to make the project's **own source** conform to
strict ISO C99 — i.e. every module compiles clean under both
`gcc -std=c99 -pedantic-errors` and `clang -std=c99 -pedantic-errors`, with no
GNU extensions or non-standard library functions.

I established ground truth by compiling every `src/*.c` under
`gcc/clang -std=c99 -pedantic-errors -Iinclude`. The violations are few and
bounded — there are exactly **three** root causes (a fourth, `__attribute__`, is a
GNU extension that both compilers tolerate but which is not portable ISO C99):

| # | Issue | Standard status | Sites |
|---|-------|-----------------|-------|
| 1 | `strdup` is POSIX, not ISO C99 → implicit-decl + pointer-from-int errors | not in C99 | `src/lexer.c` (1), `src/preprocessor.c` (6) |
| 2 | `typedef struct ASTNode ASTNode;` redefined | C11 feature, illegal in C99 | `include/codegen.h:84` |
| 3 | `__attribute__((...))` GNU specifier | GNU extension, not ISO C99 | `include/util.h` (3), `src/util.c` (3) |
| 4 | Build configured for C11 + GNU extensions | — | `CMakeLists.txt` |

All the cascading `incompatible integer to pointer conversion` errors are
downstream of #1 (undeclared `strdup` defaults to returning `int`); fixing #1
removes them.

## Approach

### 1. Replace `strdup` with an ISO-C99 helper in `util`

`strdup` is not in ISO C99. Rather than request POSIX via a feature-test macro
(`_POSIX_C_SOURCE`), which would weaken "strict ISO C99", provide our own helper
in the existing util module and route all calls through it.

- Add to `include/util.h`: `char *util_strdup(const char *s);`
- Add to `src/util.c`: implement with `malloc` + `memcpy` (handle `NULL`/length
  exactly like `strdup`). `src/util.c` already includes `<string.h>`/`<stdlib.h>`
  for its existing helpers, so no new includes are needed.
- Replace the 7 `strdup(...)` call sites in `src/lexer.c` and
  `src/preprocessor.c` with `util_strdup(...)`. Both files already include
  `util.h` (verify and add the include if missing).

Avoid naming the helper `strdup` — on glibc that name is declared once a feature
macro is present and would risk a conflicting redeclaration. `util_strdup`
sidesteps that entirely.

### 2. Remove the duplicate `ASTNode` typedef

`include/codegen.h:84` has `typedef struct ASTNode ASTNode;`, but `codegen.h`
already `#include "ast.h"` at line 5, and `ast.h` fully defines
`typedef struct ASTNode { ... } ASTNode;`. The forward typedef is redundant and
illegal under C99 (redefinition of a typedef is a C11 feature).

- Delete the redundant `typedef struct ASTNode ASTNode;` line from
  `include/codegen.h`. The type remains available via the `ast.h` include.

### 3. Make `__attribute__` portable via guarded macros

`__attribute__((noreturn))` / `((format(printf,...)))` are GNU extensions. Keep
the useful diagnostics on GNU/clang but compile clean everywhere by wrapping them
in macros that expand to nothing on non-GNU compilers.

- Add to `include/util.h` (near the top, before the declarations):
  ```c
  #if defined(__GNUC__)
  #  define CC_NORETURN      __attribute__((noreturn))
  #  define CC_PRINTF(a, b)  __attribute__((format(printf, a, b)))
  #else
  #  define CC_NORETURN
  #  define CC_PRINTF(a, b)
  #endif
  ```
- Replace the 3 attribute uses in `include/util.h` and the 3 in `src/util.c`
  with `CC_NORETURN` / `CC_PRINTF(...)`.

Note: both gcc and clang accept bare `__attribute__` even under
`-pedantic-errors` (reserved-namespace identifier), so this step is not strictly
required to make the chosen target pass — but it is the correct move for "strict
ISO C99" portability and is low-cost. Include unless you'd rather defer it.

### 4. Lock the build to strict C99

In `CMakeLists.txt`:

- Change `set(CMAKE_C_STANDARD 11)` → `set(CMAKE_C_STANDARD 99)`.
- Keep `set(CMAKE_C_STANDARD_REQUIRED ON)`.
- Add `set(CMAKE_C_EXTENSIONS OFF)` — **this is the key line**: it makes CMake emit
  `-std=c99` instead of `-std=gnu99`, actually enforcing strict mode.
- Add enforcement flags so regressions are caught going forward:
  `target_compile_options(ccompiler PRIVATE -Wall -Wextra -pedantic)`
  (use `-pedantic`, not `-pedantic-errors`, to avoid breaking the build on a
  future stray warning; the verification script below uses `-pedantic-errors`).

## Critical files

- `include/util.h` — add `util_strdup` decl, `CC_NORETURN`/`CC_PRINTF` macros, swap attributes
- `src/util.c` — implement `util_strdup`, swap attributes
- `src/lexer.c`, `src/preprocessor.c` — `strdup` → `util_strdup`
- `include/codegen.h` — delete redundant `ASTNode` typedef
- `CMakeLists.txt` — C99 + extensions off + warning flags

## Verification

1. **Strict per-module check** (the acceptance gate):
   ```bash
   for f in src/*.c; do
     gcc   -std=c99 -pedantic-errors -Iinclude -DVERSION_BUILD=0 -fsyntax-only "$f" || echo "FAIL gcc $f"
     clang -std=c99 -pedantic-errors -Iinclude -DVERSION_BUILD=0 -fsyntax-only "$f" || echo "FAIL clang $f"
   done
   ```
   Expect zero errors from both compilers.
2. **Full build** still succeeds:
   ```bash
   cmake -S . -B build && cmake --build build
   ```
3. **Behavior unchanged** — run the existing suite:
   ```bash
   ./test/run_all_tests.sh
   ```
   All previously-passing tests must still pass (the `util_strdup` swap and
   typedef removal are behavior-preserving).

## Out of scope

Self-hosting (compiling the project with ClaudeC99 itself) is explicitly **not**
part of this change. Items in `docs/self-compilation-report.md` such as adjacent
string-literal concatenation, `\xNN` hex escapes, 2D array members, and missing
stub headers are valid ISO C99 already (or are stub/compiler gaps), so they are
not source-compliance issues and are left for the compiler feature backlog.
