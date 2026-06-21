# Stage 160 Kickoff — `sizeof(expression)` in Constant Expressions and SDL2 Integration Test

## Summary of the spec

Stage 160 enables compilation of SDL2 headers by fixing a remaining parse error: `sizeof` applied to expressions (not just type names) within constant-expression contexts. The SDL2 header `SDL_events.h` contains declarations like:

```c
Uint8 padding[sizeof(void *) <= 8 ? 56 : sizeof(void *) == 16 ? 64 : 3 * sizeof(void *)];
```

and:

```c
SDL_COMPILE_TIME_ASSERT(SDL_Event, sizeof(SDL_Event) == sizeof(((SDL_Event *)NULL)->padding));
```

In C99 mode, the `SDL_COMPILE_TIME_ASSERT` macro expands to an array typedef with a constant expression that includes `sizeof(((SDL_Event *)NULL)->padding)` — a `sizeof` applied to a member-access expression rather than a type name.

Currently, `eval_const_primary()` in `src/parser.c` handles `sizeof(type-name)` but fails when the argument is an expression. This stage fixes that gap and establishes the infrastructure for optional-library integration tests (starting with SDL2).

## Required tokenizer changes

None.

## Required parser changes

Modify `eval_const_primary()` in `src/parser.c` to handle `sizeof(expr)`:

1. When `sizeof(` is followed by a non-type-name token, parse an expression instead of erroring out.
2. Extract the size from the resulting AST node's `full_type` field; fall back to `decl_type` with a size map if `full_type` is unavailable.
3. Free the AST node after extracting the size.
4. Expect `)` and return the size.

The fix replaces the final `PARSER_ERROR` in the `sizeof` handling block with an expression-parsing path that correctly handles struct member access (e.g., `((SDL_Event *)NULL)->padding`) and scalar expressions.

## Required AST changes

None.

## Required code generation changes

None.

## Test infrastructure changes

1. Create `test/integration_sysinclude/` directory for tests requiring system headers and optional third-party libraries.
2. Create `test/integration_sysinclude/run_tests.sh` runner that:
   - Checks for `.require` companion files before running tests
   - Skips tests when prerequisites are not met (reported as `SKIP`)
   - Reports `Results: P passed, F failed, S skipped, T total`
3. Add `.require` support to `test/integration/run_tests_sysinclude.sh`.
4. Update `test/run_all_tests.sh` to invoke the new sysinclude-optional runner on Linux x86_64.
5. Update `README.md` with an optional library tests table.

## Implementation tasks

### 1. Fix `eval_const_primary()` in `src/parser.c`

- Locate the `sizeof` handling block (~line 3099–3134)
- After the type-name branch, add an expression-parsing path:
  - Call `parse_expression()` to parse the argument
  - Extract size from `expr_node->full_type` or fall back to `decl_type` with a scalar size map
  - Free the AST node
  - Expect `)` and return the size
- Verify bootstrap compatibility: all required functions and headers are already available

### 2. Add portable regression tests for `sizeof(expr)`

Create two integration tests under `test/integration/`:

1. **test_sizeof_expr_ptr** — `sizeof` applied to member access through a null cast:
   - Verifies: `sizeof(((struct Box *)0)->label)` == 20
   - Expected: exit status 42

2. **test_sizeof_expr_array_dim** — `sizeof(expr)` used as an array dimension:
   - Verifies: `sizeof(void *) <= 8 ? 16 : 32` pattern (SDL2 style)
   - Expected: exit status 42

Both tests should pass on all platforms without requiring system headers.

### 3. Create `test/integration_sysinclude/` infrastructure

- Create directory `test/integration_sysinclude/`
- Create `run_tests.sh` runner that:
  - Uses system include paths (not `test/include` stubs)
  - Checks for `<name>.require` before running each test
  - Runs `.require` command silently and skips test if it fails
  - Reports `SKIP  <name>  (requires: <cmd>)` for skipped tests
  - Reports final line: `Results: P passed, F failed, S skipped, T total`
- Update `test/integration/run_tests_sysinclude.sh` to add `.require` checks

### 4. Add SDL2 integration test

Create `test/integration_sysinclude/test_sdl2_init/`:

- **test_sdl2_init.c** — minimal SDL2 program:
  - Includes `<SDL2/SDL.h>` and `<stdio.h>`
  - Calls `SDL_Init(0)`, `SDL_GetVersion()`, `SDL_Quit()`
  - Prints SDL2 version
  - Returns exit status 42
- **test_sdl2_init.require** — prerequisite check:
  - Command: `sdl2-config --version`
  - Test is skipped if SDL2 is not installed
- **test_sdl2_init.cflags** — compiler flags:
  - `-DSDL_DISABLE_IMMINTRIN_H` (skip GCC intrinsic headers)
- **test_sdl2_init.libs** — linker flags:
  - `-lSDL2`
- **test_sdl2_init.status** — expected exit status:
  - `42`
- No `.expected` file (version output varies by installation)

### 5. Update `test/run_all_tests.sh`

- Add `test/integration_sysinclude/run_tests.sh` to the test suite
- Gate sysinclude-optional tests on Linux x86_64 (same as existing `run_tests_sysinclude.sh`)
- Include skipped test counts in final results

### 6. Update `README.md`

- Add new `## Optional library tests` section after existing test suite description
- Create table with columns: Library, Install command (Debian/Ubuntu), Prerequisite check, Test(s)
- List SDL2: `apt install libsdl2-dev`, `sdl2-config`, `test_sdl2_init`

### 7. Update version

- Change `VERSION_STAGE` in `src/version.c` from `"01590000"` to `"01600000"`

### 8. Test and validate

- Run `./test/integration/run_tests.sh` — all portable tests pass
- Run `./test/integration/run_tests_sysinclude.sh` — sysinclude tests pass (with `.require` support)
- Run `./test/integration_sysinclude/run_tests.sh` — SDL2 test passes or is skipped
- Run `./test/run_all_tests.sh` — all suites pass or skip appropriately
- Run `./build.sh --mode=self-host` — C0→C1→C2 verified

### 9. Commit

- Single commit with message referencing Stage 160 and the SDL2 sizeof fix
- Include the Co-Authored-By trailer

## Test plan

### Integration tests

1. **test_sizeof_expr_ptr** — member access through null cast
   - Verifies: `sizeof(((struct Box *)0)->label)` and `sizeof(((struct Box *)0)->w)` in assignments
   - Expected: exit status 42

2. **test_sizeof_expr_array_dim** — `sizeof(expr)` as array dimension
   - Verifies: ternary with `sizeof(void *)` as array dimension
   - Expected: exit status 42

3. **test_sdl2_init** — real SDL2 program
   - Verifies: cc99 can compile and link a minimal SDL2 program end-to-end
   - Expected: exit status 42 (or SKIP if SDL2 not installed)

### Regression testing

- Full test suite (`./test/run_all_tests.sh`) must pass
- All existing parser tests unaffected
- Stage 159 tests (function pointer array parameters) continue to pass
- Portable tests pass on all platforms
- sysinclude tests pass with system headers
- Optional sysinclude tests (SDL2) pass or skip based on `.require` check
- Self-hosting: C0→C1→C2 bootstrap remains valid

## Out of scope

- `sizeof(variable)` in constant expression contexts — variable types are not available at the parser level; runtime codegen fallback handles this
- GCC vector intrinsic headers (`xmmintrin.h`, `bmi2intrin.h`, etc.) — require `unsigned __int128` and GCC vector types; users must use `-DSDL_DISABLE_IMMINTRIN_H`
- Full SDL2 program with window/renderer — requires a display server; not suitable for automated testing
- Other optional-library tests — future stages may extend the `.require` infrastructure for zlib, OpenGL, etc.
