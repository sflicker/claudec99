# ClaudeC99 Stage 160 — `sizeof(expression)` in Constant Expressions and SDL2 Integration Test

## Background

Stage 159 fixed the primary SDL2 compile error and several GCC extension
parsing issues.  With the additional workaround flag `-DSDL_DISABLE_IMMINTRIN_H`
(which skips GCC vector-intrinsic headers that use language features well
beyond C99), compilation of SDL2 headers advances to a single remaining error:

```
/usr/include/SDL2/SDL_events.h:669:76: error: sizeof requires a type name
in a constant expression context
```

The failing declaration is:

```c
Uint8 padding[sizeof(void *) <= 8 ? 56 : sizeof(void *) == 16 ? 64 : 3 * sizeof(void *)];
```

followed by:

```c
SDL_COMPILE_TIME_ASSERT(SDL_Event, sizeof(SDL_Event) == sizeof(((SDL_Event *)NULL)->padding));
```

In C99 mode `SDL_COMPILE_TIME_ASSERT` expands to:

```c
typedef int SDL_compile_time_assert_SDL_Event[
    (sizeof(SDL_Event) == sizeof(((SDL_Event *)NULL)->padding)) * 2 - 1
];
```

`sizeof(((SDL_Event *)NULL)->padding)` applies `sizeof` to a member-access
**expression** rather than to a type name.  `eval_const_primary` in
`src/parser.c` supports `sizeof(type-name)` but errors out on any non-type
argument.

This stage fixes that gap and adds a minimal SDL2 integration test that
verifies cc99 can compile and link a real SDL2 program.  It also establishes
the infrastructure for optional-library integration tests so future stages can
add similar tests without breaking the standard test run on machines that lack
those libraries.

---

## Goals

1. **Fix `sizeof(expr)` in compile-time constant expressions** — extend
   `eval_const_primary` in `src/parser.c` so that when `sizeof(` is followed
   by a non-type-name token, the expression inside the parentheses is parsed,
   its type is resolved from the resulting AST node, and the correct size is
   returned.

2. **Add optional-library test infrastructure** — introduce a
   `test/integration_sysinclude/` directory for tests that require both system
   headers and potentially optional third-party libraries.  Add a `.require`
   companion-file convention so tests can declare a shell prerequisite check;
   if the check fails, the test is counted as SKIP rather than FAIL.

3. **Add an SDL2 integration test** — `test_sdl2_init`: compiles and runs a
   minimal SDL2 program (init / version query / quit) using system headers and
   the SDL2 shared library.  The test is skipped automatically on machines
   where SDL2 is not installed.

4. **Document optional library tests in `README.md`** — add a table of
   optional libraries used by integration tests with install commands.

---

## Part 1 — Fix `sizeof(expr)` in `eval_const_primary`  (`src/parser.c`)

### Current behaviour (lines ~3099–3134)

```c
if (parser->current.type == TOKEN_SIZEOF) {
    parser->current = lexer_next_token(parser->lexer);
    if (parser->current.type != TOKEN_LPAREN)
        PARSER_ERROR(...);
    parser->current = lexer_next_token(parser->lexer); /* consume '(' */
    if (/* token is a type-name start */) {
        Type *t = parse_type_name(parser);
        ...
        parser_expect(parser, TOKEN_RPAREN);
        return (long)type_size(t);
    }
    PARSER_ERROR(parser,
        "error: sizeof requires a type name in a constant expression context\n");
}
```

### Fix

Replace the final `PARSER_ERROR` with an expression-parsing path:

```c
    /* sizeof(expr): parse the expression, derive size from the node's type. */
    {
        ASTNode *expr_node;
        long sz;
        expr_node = parse_expression(parser);
        if (expr_node != NULL && expr_node->full_type != NULL) {
            sz = (long)type_size(expr_node->full_type);
        } else {
            /* Scalar fallback: use decl_type alone. */
            int k = (expr_node != NULL) ? expr_node->decl_type : TYPE_INT;
            if (k == TYPE_POINTER || k == TYPE_LONG || k == TYPE_LONG_LONG ||
                k == TYPE_UNSIGNED_LONG_LONG || k == TYPE_DOUBLE)
                sz = 8;
            else if (k == TYPE_SHORT)
                sz = 2;
            else if (k == TYPE_CHAR || k == TYPE_BOOL)
                sz = 1;
            else
                sz = 4; /* TYPE_INT, TYPE_FLOAT, enum, etc. */
        }
        if (expr_node != NULL) ast_free(expr_node);
        parser_expect(parser, TOKEN_RPAREN);
        return sz;
    }
```

### Why this works for the SDL2 case

`((SDL_Event *)NULL)->padding` is a struct-member access.  The parser handles
`->member` by looking up the field in the struct type table, producing an AST
node whose `full_type` points to the `Uint8[56]` array type (with
`full_type->size == 56`).  `type_size(full_type)` therefore returns 56, which
is the correct sizeof value.

For `sizeof(SDL_Event)` — already in the type-name branch (SDL_Event is a
typedef), so unchanged.

### Bootstrap caveat

`parse_expression` is already declared and defined within `src/parser.c`.
`ast_free` and `type_size` are declared in `include/ast.h` and `include/type.h`
respectively — both already included.  All new variable declarations (`expr_node`,
`sz`, `k`) must appear before any executable statements in the block.
No VLAs; no `//` comments.

---

## Part 2 — Optional-library test infrastructure

### New directory: `test/integration_sysinclude/`

Create `test/integration_sysinclude/` to hold tests that must have:
- Full system include paths (not the `test/include` stubs), **and**
- Possibly optional third-party libraries (`-lSDL2`, `-lz`, etc.)

The directory follows the same companion-file conventions as
`test/integration/` (`.c`, `.expected`, `.status`, `.cflags`, `.libs`,
`.args`, `.input`, `.error`) with one addition:

### New companion file: `.require`

If a test directory contains `<name>.require`, the runner reads it as a shell
command and runs it silently.  If the command exits non-zero, the test is
**skipped** (reported as `SKIP` and excluded from both the pass and fail
counts, but included in a separate skip total).

Example for the SDL2 test:

```
sdl2-config --version
```

If `sdl2-config` is not on PATH (SDL2 not installed), the command fails, and
the test is skipped with no error.

### New runner: `test/integration_sysinclude/run_tests.sh`

Create `test/integration_sysinclude/run_tests.sh`.  It is identical to
`test/integration/run_tests_sysinclude.sh` in how it compiles and links
(using the system include paths), but it:

- Iterates over subdirectories of `test/integration_sysinclude/` (not
  `test/integration/`)
- Checks for `<name>.require` before doing anything else; skips if it fails
- Reports `SKIP  <name>  (requires: <cmd>)` for skipped tests
- Prints a final line of the form:
  `Results: P passed, F failed, S skipped, T total`

### Update `test/run_all_tests.sh`

Add `test/integration_sysinclude/run_tests.sh` to the list of suite runners.
The sysinclude-optional suite is only run on Linux x86_64 (same gate as the
existing `run_tests_sysinclude.sh`).  Skipped tests are not counted in the
portable or system-include totals — they get their own section line.

### `.require` in the existing `run_tests_sysinclude.sh`

Add the same `.require` check to `test/integration/run_tests_sysinclude.sh`
so that if any test in `test/integration/` ever acquires a `.require` file,
it is respected.

---

## Part 3 — SDL2 integration test

### `test/integration_sysinclude/test_sdl2_init/`

**`test_sdl2_init.c`**

```c
/* Minimal SDL2 smoke-test: init (no subsystems), query version, quit.
 * Compile: --sysinclude -DSDL_DISABLE_IMMINTRIN_H
 * Link:    -lSDL2  */
#include <SDL2/SDL.h>
#include <stdio.h>

int main(void) {
    SDL_version ver;

    if (SDL_Init(0) != 0)
        return 1;

    SDL_GetVersion(&ver);
    printf("SDL2 %d.%d.%d\n", (int)ver.major, (int)ver.minor, (int)ver.patch);

    SDL_Quit();
    return 42;
}
```

**`test_sdl2_init.require`**

```
sdl2-config --version
```

**`test_sdl2_init.cflags`**

```
-DSDL_DISABLE_IMMINTRIN_H
```

**`test_sdl2_init.libs`**

```
-lSDL2
```

**`test_sdl2_init.status`**

```
42
```

**`test_sdl2_init.expected`** — the SDL2 version string varies by installation,
so do NOT include an `.expected` file.  The test verifies only the exit status.

### How the runner compiles this test

`run_tests.sh` in `test/integration_sysinclude/` uses the same system include
paths as `run_tests_sysinclude.sh`:

```
-I/usr/lib/gcc/x86_64-linux-gnu/13/include
-I/usr/local/include
-I/usr/include/x86_64-linux-gnu
-I/usr/include
```

combined with the `.cflags` content (`-DSDL_DISABLE_IMMINTRIN_H`).  The linker
step passes `extra_libs` (`-lSDL2`).

---

## Part 4 — README update

Add a new `## Optional library tests` section to `README.md`, positioned after
the existing test-suite description, listing libraries that some integration
tests depend on:

```markdown
## Optional library tests

Integration tests under `test/integration_sysinclude/` may require optional
system libraries.  Tests for missing libraries are automatically skipped
(reported as `SKIP`, not `FAIL`).  The `.require` companion file in each test
directory names the prerequisite check command.

| Library | Debian/Ubuntu install     | Prerequisite check | Test(s)           |
|---------|---------------------------|--------------------|-------------------|
| SDL2    | `apt install libsdl2-dev` | `sdl2-config`      | `test_sdl2_init`  |
```

Future stages that add tests for other libraries (zlib, OpenGL, etc.) append
rows to this table.

---

## Version

Update `src/version.c`:

```c
#define VERSION_STAGE  "01600000"
```

---

## Tests

### New test

`test/integration_sysinclude/test_sdl2_init` — verifies cc99 can compile and
link a real SDL2 program end-to-end.  Skipped when SDL2 is not installed.

### Regression tests for `sizeof(expr)`

Add two integration tests to `test/integration/` (portable suite, no system
headers needed):

**`test_sizeof_expr_ptr`** — `sizeof` applied to a member access through a cast:

```c
struct Box { int w; int h; char label[20]; };
int main(void) {
    /* sizeof(((struct Box *)0)->label) == 20 */
    int a = sizeof(((struct Box *)0)->label);
    /* sizeof(((struct Box *)0)->w) == 4 */
    int b = sizeof(((struct Box *)0)->w);
    if (a != 20) return 1;
    if (b != 4)  return 2;
    return 42;
}
```

`.status`: `42`

**`test_sizeof_expr_array_dim`** — `sizeof(expr)` used directly as an array
dimension (the SDL2 pattern):

```c
struct Pad {
    int tag;
    char data[sizeof(void *) <= 8 ? 16 : 32];
};
int main(void) {
    if (sizeof(((struct Pad *)0)->data) != 16) return 1;
    return 42;
}
```

`.status`: `42`

---

## Out of scope

- `sizeof(variable)` in constant expression context — variable types are not
  available at the parser level.  Existing codegen fallback handles runtime
  sizeof.
- GCC vector intrinsic headers (`xmmintrin.h`, `bmi2intrin.h`, `sgxintrin.h`)
  — require `unsigned __int128` and GCC vector types; out of scope for a C99
  compiler.  Users must pass `-DSDL_DISABLE_IMMINTRIN_H`.
- Full SDL2 program with window/renderer — requires a display server;
  not suitable for automated testing.

---

## Implementation order

1. Fix `eval_const_primary` in `src/parser.c` (sizeof expr path).
2. Add two portable integration tests (`test_sizeof_expr_ptr`,
   `test_sizeof_expr_array_dim`) and verify they pass.
3. Create `test/integration_sysinclude/` directory with
   `run_tests.sh` runner and the `.require` mechanism.
4. Add `test_sdl2_init` to `test/integration_sysinclude/`.
5. Add `.require` check to `test/integration/run_tests_sysinclude.sh`.
6. Update `test/run_all_tests.sh` to invoke the new runner on Linux x86_64.
7. Update `README.md` with the optional library table.
8. Bump `VERSION_STAGE` to `"01600000"` in `src/version.c`.
9. Run `./test/run_all_tests.sh` — all portable tests pass; SDL2 test passes
   or is skipped depending on environment.
10. Run `./build.sh --mode=self-host` — C0→C1→C2 verified.
