# Stage 56-02 Kickoff: Command-line Macro Definitions

## Spec Summary

Stage 56-02 adds support for defining preprocessor macros via command-line `-D` flags. The compiler now accepts:

- `-DNAME` — behaves as `#define NAME 1`
- `-DNAME=VAL` — behaves as `#define NAME VAL`

These macros must be available before preprocessing the main source file, so they work correctly with preprocessor conditionals like `#ifdef DEBUG` and `#if LEVEL >= 2`.

## Tokenizer Changes

**None.** The tokenizer does not process command-line arguments; macros are injected into the preprocessor.

## Parser Changes

**None.** Parser behavior is unchanged; macros are resolved during preprocessing.

## AST Changes

**None.** No new AST nodes are required.

## Code Generation Changes

**None.** Code generation is unaffected; macros are resolved during preprocessing.

## Preprocessor Changes

### Summary

Add a public function `preprocess_with_defines()` that accepts an array of define strings and injects them as preprocessor macros before processing the source file.

### Implementation Steps

1. **Add public API in `include/preprocessor.h`:**
   - `preprocess_with_defines(source, source_path, defines, n_defines)`
   - `source`: source code string
   - `source_path`: filename for diagnostics
   - `defines`: array of define strings (e.g., `["LEVEL=3", "DEBUG"]`)
   - `n_defines`: number of defines in the array
   - Returns: preprocessed source as a string

2. **Implement in `src/preprocessor.c`:**
   - Parse each define string to extract name and optional value
   - Format as synthetic `#define` directives
   - Prepend to source before calling existing preprocessor
   - Handle both `-DNAME` and `-DNAME=VAL` formats

## Driver Changes

### Summary

Update the compiler driver (`src/compiler.c`) to parse command-line arguments, collect `-D` definitions, and pass them to the new preprocessor function.

### Implementation Steps

1. **Parse argv for `-D` flags:**
   - Scan all arguments for `-D` prefix
   - Extract `NAME` from `-DNAME` and `NAME=VAL` from `-DNAME=VAL`
   - Collect into a dynamic array of define strings

2. **Call `preprocess_with_defines()`:**
   - Pass collected defines to the preprocessor
   - Use returned preprocessed source for subsequent compilation stages

## Integration Test Framework Changes

### Summary

Add `.cflags` companion file support to the integration test runner to pass compiler flags (not runtime arguments).

### Context

The spec originally referenced `.args` files for compiler definitions, but `.args` is already used in the integration suite to pass runtime arguments to compiled programs (see `test_argv_puts`). To avoid conflict, `.cflags` will be used for compiler flags instead.

### Implementation Steps

1. **Update test runner** to recognize `.cflags` companion files
2. **Extract compiler flags** from `.cflags` and add to compiler invocation
3. **Maintain separation** between `.cflags` (compiler flags) and `.args` (runtime arguments)

## Test Plan

Create two integration tests matching the spec's test cases:

### 1. Test: `test_cmdline_define_level`

**Companion file:** `test_cmdline_define_level.cflags`

```
-DLEVEL=3
```

**Test file:** `test/valid/test_cmdline_define_level.c`

```c
#if LEVEL == 3
int main() { return 42; }
#else
int main() { return 1; }
#endif
```

**Expected:** Compilation succeeds; program returns 42.

### 2. Test: `test_cmdline_define_debug`

**Companion file:** `test_cmdline_define_debug.cflags`

```
-DDEBUG
```

**Test file:** `test/valid/test_cmdline_define_debug.c`

```c
#ifdef DEBUG
int main() { return 42; }
#else
int main() { return 1; }
#endif
```

**Expected:** Compilation succeeds; program returns 42.

## Implementation Order

1. Add `preprocess_with_defines()` public API in `include/preprocessor.h`
2. Implement `preprocess_with_defines()` in `src/preprocessor.c`
3. Update compiler driver (`src/compiler.c`) to parse `-D` flags and call new function
4. Update integration test runner to support `.cflags` companion files
5. Create integration test files with `.cflags` companions
6. Run tests and verify
7. Update `README.md` with stage completion

## Notes

- The new preprocessor function should construct synthetic `#define` directives from command-line arguments and prepend them to the source before preprocessing.
- Both `-DNAME` (implying value `1`) and `-DNAME=VAL` formats must be handled.
- The `.cflags` companion file format simplifies testing and avoids conflict with existing `.args` usage.
