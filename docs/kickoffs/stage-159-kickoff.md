# Stage 159 Kickoff — SDL2 Compatibility: Array Parameters in Function Pointer Types

## Summary of the spec

Stage 159 fixes a compile failure when processing SDL2 headers, specifically `/usr/include/SDL2/SDL_main.h:152:50: error: expected ')', got '[' ('[')`.

The root cause: SDL_main.h contains a typedef for a function pointer type with array parameters:

```c
typedef int (*SDL_main_func)(int argc, char *argv[]);
```

When parsing this declaration, the inline parameter-type parsing loops inside `parse_declarator()` (in `src/parser.c`) for function pointer types consume optional identifier names but do not handle `[]` array suffixes. After consuming the identifier `argv`, the parser encounters `[`, which is neither `,` nor `)`, so it breaks out of the loop and then fails trying to expect `)`.

Per C99 §6.7.5.3p7, array parameters in function declarations automatically adjust to pointers. The parser must recognize and consume `[...]` bracket suffixes after parameter names, and add one pointer level per bracket consumed.

## Required tokenizer changes

None.

## Required parser changes

Fix three inline parameter-type parsing loops within `parse_declarator()` in `src/parser.c`. After consuming the optional parameter name in each loop, add logic to:
1. Check for and consume `[` tokens
2. Skip the array dimension expression (if present)
3. Consume the matching `]` token
4. Add one pointer level to the declaration for each array consumed (per C99 array-to-pointer decay rule)

The three locations are:
1. The main function-pointer parameter loop (~line 1583)
2. The "own params" loop for functions-returning-fp (~line 1460)
3. The "returned fp params" loop for functions-returning-fp (~line 1500)

## Required AST changes

None.

## Required code generation changes

None.

## Implementation tasks

### 1. Analyze the current parse_declarator logic

- Review the three inline parameter-type parsing loops
- Identify the exact structure and control flow
- Understand how parameters are currently being parsed

### 2. Add array handling to each loop

For each of the three loops:
1. After consuming the optional parameter identifier name
2. Add a loop to consume `[...]` suffixes:
   - While next token is `[`:
     - Consume `[`
     - Skip array dimension expression (if present; dimension may be empty)
     - Consume `]`
     - Increment pointer level counter
3. After the loop exits, apply accumulated pointer levels to the parameter declaration

### 3. Test array-to-pointer conversion

- Verify that `char *argv[]` is correctly parsed as a pointer to pointer to char
- Verify that multi-dimensional arrays like `int arr[][]` are correctly handled
- Verify that named parameters with arrays work: `void f(int x[10])`

### 4. Update version stamp

- Change `VERSION_STAGE` in `src/version.c` from `"01580000"` to `"01590000"`

### 5. Integration tests

Create integration tests under `test/integration/` to verify array parameters in function pointers:

1. **test_fp_single_array_param** — function pointer with single array parameter
   - Typedef: `typedef int (*func_t)(char *argv[])`
   - Verify: typedef compiles and function pointer can be called
   - Expected: no parse errors, proper pointer-to-pointer handling

2. **test_fp_multi_array_params** — function pointer with multiple array parameters
   - Typedef: `typedef void (*func_t)(int x[], int y[10], int z[][])`
   - Verify: multiple arrays with and without dimensions
   - Expected: all array parameters correctly converted to pointers

3. **test_fp_named_array_params** — function pointer with named array parameters
   - Typedef: `typedef int (*func_t)(int argc, char *argv[])`
   - Verify: named parameters with arrays
   - Expected: compiles and execution works correctly

4. **test_fp_mixed_array_ptr_params** — function pointer mixing arrays and explicit pointers
   - Typedef: `typedef void (*func_t)(int *arr, int size, char *data[])`
   - Verify: mix of pointer and array parameters
   - Expected: correct handling of both forms

Each test should verify:
- The compiler produces no parse errors
- The resulting program compiles successfully
- Function pointers can be declared, assigned, and called with the correct signatures

### 6. Bootstrap validation

- The parser changes use only existing parser operations
- No new C constructs are needed in the parser itself
- Must remain compatible with C89/C99 bootstrapping

### 7. SDL2 smoke test (optional)

- If SDL2 is available, attempt to compile the spec example with `cc99 --sysinclude sdl2_demo.c`
- Expected: compiles successfully without parse errors

### 8. Commit

- Single commit with message referencing stage 159 and the SDL2 issue
- Include the Co-Authored-By trailer

## Test plan

### Integration tests

1. **test_fp_single_array_param** — function pointer with single array parameter
   - Verify: `typedef int (*func_t)(char *argv[])` parses correctly
   - Expected output: program compiles and runs

2. **test_fp_multi_array_params** — function pointer with multiple array parameters
   - Verify: multiple array parameters with various dimensions
   - Expected output: program compiles and runs

3. **test_fp_named_array_params** — function pointer with named array parameters
   - Verify: named parameters with arrays (the SDL2 pattern)
   - Expected output: program compiles and runs

4. **test_fp_mixed_array_ptr_params** — function pointer with mixed array and pointer parameters
   - Verify: correct handling of both `*arr` and `arr[]` in same parameter list
   - Expected output: program compiles and runs

### Regression testing

- Full test suite (`./run_tests.sh`) must pass
- All existing parser tests unaffected
- Stage 158 tests (preprocessor) continue to pass
- Smoke test: if SDL2 is available, verify `cc99 --sysinclude sdl2_demo.c -o sdl2_demo $(sdl2-config --cflags --libs)` compiles successfully

## Out of scope

- **Macro improvements** — preprocessor remains unchanged from Stage 158
- **Other SDL2 incompatibilities** — scope limited to array parameter parsing
- **VLA handling** — variable-length arrays in parameter declarations are not in scope
- **Function pointer typedef variations** — focus on parameter array handling
