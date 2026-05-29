# Stage 75-05 Kickoff: Additional va_list Tests

## Summary of the Spec

Stage 75-05 is a tests-only stage that verifies the va_start/va_list passing infrastructure from Stage 75-04 across varying variadic argument counts. This stage adds three new integration tests:

1. **test_va_start_no_varargs** — Print with no extra arguments
2. **test_va_start_6_varargs** — Print with 6 arguments (register boundary)
3. **test_va_start_10_varargs** — Print with 10 arguments (stack overflow testing)

All tests use the same pattern: a `print()` function that calls `va_start`, passes the `va_list` to `printv()`, which calls `vprintf()`. This verifies that the va_list struct correctly captures the register save area and overflow argument area across the full range of argument counts.

## Tokenizer Changes

None required.

## Parser Changes

None required.

## AST Changes

None required.

## Code Generation Changes

None required.

## Test Plan

### Test 1: test_va_start_no_varargs

**Pattern**: Calls `print()` with format string only, no extra arguments.

**Code**:
```C
#include <stdarg.h>
#include <stdio.h>

void printv(const char *fmt, va_list ap) {
    vprintf(fmt, ap);
}

void print(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    printv(fmt, ap);
    va_end(ap);
}

int main() {
    print("Hello\n");
    return 0;
}
```

**Expected output**: `Hello\n`

**Significance**: Verifies that `vprintf()` handles an empty va_list correctly (no arguments to consume).

### Test 2: test_va_start_6_varargs

**Pattern**: Calls `print()` with format string and 6 integer arguments.

**Code**:
```C
#include <stdarg.h>
#include <stdio.h>

void printv(const char *fmt, va_list ap) {
    vprintf(fmt, ap);
}

void print(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    printv(fmt, ap);
    va_end(ap);
}

int main() {
    print("%d %d %d %d %d %d\n", 1, 2, 3, 4, 5, 6);
    return 0;
}
```

**Expected output**: `1 2 3 4 5 6\n`

**Significance**: Tests the register boundary case. The format string uses `rdi`, and the six integer arguments occupy the remaining five GP registers (`rsi`, `rdx`, `rcx`, `r8`, `r9`) plus the stack (arg 6). This verifies that va_list correctly handles the transition from register save area to overflow area.

### Test 3: test_va_start_10_varargs

**Pattern**: Calls `print()` with format string and 10 integer arguments.

**Code**:
```C
#include <stdarg.h>
#include <stdio.h>

void printv(const char *fmt, va_list ap) {
    vprintf(fmt, ap);
}

void print(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    printv(fmt, ap);
    va_end(ap);
}

int main() {
    print("%d %d %d %d %d %d %d %d %d %d\n", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    return 0;
}
```

**Expected output**: `1 2 3 4 5 6 7 8 9 10\n`

**Significance**: Tests the stack overflow area. Arguments 1–5 occupy GP registers; arguments 6–10 sit on the stack in the overflow_arg_area. This verifies that va_list correctly traverses the full stack-based argument region.

## Implementation Order

1. Create integration test directory structure:
   - `test/integration/test_va_start_no_varargs/` with `.c` and `.expected` files
   - `test/integration/test_va_start_6_varargs/` with `.c` and `.expected` files
   - `test/integration/test_va_start_10_varargs/` with `.c` and `.expected` files
2. Update `src/version.c` from `"00750400"` to `"00750500"`

## Ambiguities and Decisions

- **No-varargs test significance**: Ensures that `vprintf()` does not attempt to read from va_list fields when there are no variadic arguments to consume. This is a boundary condition that should not crash or produce spurious output.
- **6-arg test register/stack boundary**: Argument 1 (value 1) goes in `rsi`, argument 2 in `rdx`, argument 3 in `rcx`, argument 4 in `r8`, argument 5 in `r9`, and argument 6 (value 6) on the stack at `[rbp + 16]`. The va_list must correctly record the transition via `overflow_arg_area`.
- **10-arg test deep stack traversal**: Arguments 6–10 (values 6–10) sit at `[rbp + 16]`, `[rbp + 24]`, `[rbp + 32]`, `[rbp + 40]`, and `[rbp + 48]` respectively. This tests that the overflow_arg_area offset and size calculations are correct beyond the first stack argument.
- **Spec note**: The spec title contains "additional additional" (a minor typo) but does not affect implementation or expected behavior.
