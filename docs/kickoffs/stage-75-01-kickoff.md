# Stage 75-01: Variadic Definition Syntax and Caller Compatibility

## Summary of the Spec

ClaudeC99 will support parsing and type-checking variadic function **definitions**. Functions may be declared with trailing ellipsis (`...`) to accept variable numbers of arguments beyond a fixed parameter count. Callers can pass more arguments than the declared fixed parameters; callees currently ignore extra arguments (no stdarg.h, va_list, va_start, or va_arg support yet).

**Key concepts:**
- Variadic definitions: `int f(int fixed, ...)` with function body
- Variadic prototypes: `int f(int x, ...);`
- Caller rule: For variadic functions, actual argument count must be `>= fixed_param_count` (instead of exactly equal for non-variadic functions)
- Unnamed parameters are allowed in variadic definitions

## Required Tokenizer Changes

**None.** TOKEN_ELLIPSIS already exists and is recognized.

## Required Parser Changes

Relax the unnamed-parameter check in function definitions. Currently, ClaudeC99 rejects unnamed parameters in function definitions. For variadic functions, unnamed fixed parameters must be allowed. The parser should:

1. Continue to allow unnamed parameters only in function definitions (not prototypes or regular function signatures)
2. Specifically allow unnamed parameters in variadic function definitions
3. Preserve existing restrictions for non-variadic function definitions (if any)

## Required AST Changes

**None.** The `is_variadic` field already exists on function AST nodes. The `fixed_param_count` is implicitly the number of parameters before the ellipsis.

## Required Code Generation Changes

1. **Function prologue:** Skip code generation for unnamed parameters. Do not call `codegen_add_var` or emit `mov` instructions for unnamed fixed parameters in variadic functions.

2. **Function calls:** For calls to variadic functions, emit `xor eax, eax` before the call (this is consistent with existing handling of external variadic calls, since %al carries the count of vector registers used for floating-point arguments, which is always zero in ClaudeC99).

3. **7+ arguments:** Apply existing stack-passing rules when total argument count is 7 or more (already implemented in stage 68).

4. **Version:** Update VERSION_STAGE in `src/version.c` to `"00750100"`.

## Test Plan

### Valid Tests (expect successful compilation and execution)

1. **Fixed parameter return:** Single fixed parameter with variadic marker. Function ignores extra arguments and returns the fixed parameter.
   - Input: `fixed(42, 1, 2, 3)`
   - Expected return: `42`

2. **Two-parameter sum:** Two fixed parameters with variadic marker. Function sums the fixed parameters, ignoring extra arguments.
   - Input: `sum(1, 2, 3, 4, 5)`
   - Expected return: `3`

3. **Unnamed parameter with 7+ arguments:** Named string parameter (or unnamed pointer) with variadic marker, testing stack-passing behavior.
   - Input: `log("hello", 1, 2, 3, 4, 5, 6, 7, 8, 9)`
   - Expected return: `1`

### Invalid Tests (expect compilation failure)

1. **Too few fixed arguments:** Call variadic function with fewer than the required fixed parameters.
   - Input: `f()` where `f(int x, ...)` is declared
   - Expected: Error (missing required fixed argument)

2. **Insufficient fixed parameters (two-param case):** Call variadic function with only one argument when two fixed parameters are required.
   - Input: `sum(1)` where `sum(int a, int b, ...)` is declared
   - Expected: Error (missing required fixed argument)

---

**Spec Notes:** The provided spec has three minor issues (missing closing braces at lines 25 and 94, typo "reutrn" at line 102) that should be corrected before finalizing this stage.
