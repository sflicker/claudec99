# Stage 75-06 Kickoff: va_arg for Integer and Pointer Types

## Summary of the Spec

Stage 75-06 implements code generation for `va_arg(ap, type)` on x86-64 for general purpose (GP) register class types. The parser already supports `__builtin_va_arg` from Stage 75-03; this stage replaces the codegen stub with the complete SysV AMD64 va_arg algorithm.

**Scope**: Support va_arg for:
- Integer types: `int`, `unsigned int`, `long`, `unsigned long`, `long long`, `unsigned long long`
- Pointer types: any pointer, including function pointers and struct/union pointers

**Unsupported types** (with validation errors):
- Small/promoted types: `char`, `short`, `_Bool` (users must cast)
- Floating-point: `float`, `double`
- Aggregates by value: `struct` and `union` (pointers to aggregates are OK)
- Array types, `void`

**ABI Model**: Uses the existing va_list layout:
- `gp_offset` (offset 0, 4 bytes): index into register save area (0–47)
- `fp_offset` (offset 4, 4 bytes): index into FP register save area (unused in this stage)
- `overflow_arg_area` (offset 8, 8 bytes): pointer to stack overflow region
- `reg_save_area` (offset 16, 8 bytes): pointer to saved GP/FP registers

For integer/pointer types, the load sequence is:
1. Fetch `gp_offset` from `ap[0].gp_offset`
2. If `gp_offset < 48`, load from `ap[0].reg_save_area + gp_offset` and increment `gp_offset` by 8
3. If `gp_offset >= 48`, load from `ap[0].overflow_arg_area` and increment that pointer by 8
4. Load the value of the requested type from the computed address

## Tokenizer Changes

None required.

## Parser Changes

None required. The parser already supports `__builtin_va_arg(ap, type)` from Stage 75-03.

## AST Changes

None required. `AST_BUILTIN_VA_ARG` already exists with two operands (ap expression, type).

## Code Generation Changes

**File**: `src/codegen.c`

Replace the stub implementation in the `AST_BUILTIN_VA_ARG` handler with the full SysV AMD64 va_arg algorithm:

1. **Type validation**: Reject small promoted types (char, short, _Bool), floating-point, aggregates by value, arrays, and void. Accept all integer types, pointer types, and pointers to aggregates.

2. **Algorithm**:
   - Load `gp_offset` from `ap[0]` offset 0 (4-byte unsigned int)
   - Compare against 48
   - If less than 48: load from `ap[0].reg_save_area + gp_offset`, update `gp_offset` by +8
   - If greater or equal: load from `ap[0].overflow_arg_area`, advance that pointer by +8
   - Convert loaded 8-byte value to the target type (sign/zero extend for smaller types like int)

3. **Load behavior**:
   - `int` / `unsigned int`: load 4 bytes (sign or zero extend to 64-bit)
   - `long` / `unsigned long`: load 8 bytes
   - `long long` / `unsigned long long`: load 8 bytes
   - Pointer types: load 8 bytes

**Version Update**: `src/version.c` — update to `"00750600"`

## Test Plan

### Valid Tests (6 new)

#### Test 1: test_va_arg_int_sum3
**Pattern**: Basic integer accumulation from register save area.

**Source**:
```C
#include <stdarg.h>

int sum3(int first, ...) {
    va_list ap;
    int total;

    total = first;
    va_start(ap, first);
    total += va_arg(ap, int);
    total += va_arg(ap, int);
    va_end(ap);
    
    return total;
}

int main(void) {
    return sum3(10, 20, 12);
}
```

**Expected return**: `42` (10 + 20 + 12)

**Significance**: Tests basic va_arg for int from the register save area.

#### Test 2: test_va_arg_pick7_overflow
**Pattern**: Transition from register save area to overflow stack area.

**Source**:
```C
#include <stdarg.h>

int pick7(int fixed, ...) {
    va_list ap;
    int a, b, c, d, e, f, g;
    
    va_start(ap, fixed);
    
    a = va_arg(ap, int);
    b = va_arg(ap, int);
    c = va_arg(ap, int);
    d = va_arg(ap, int);
    e = va_arg(ap, int);
    f = va_arg(ap, int);
    g = va_arg(ap, int);
    
    va_end(ap);
    
    return g;
}

int main(void) {
    return pick7(0, 1, 2, 3, 4, 5, 6, 42);
}
```

**Expected return**: `42`

**Significance**: Tests the boundary between register save area (6 GP regs) and overflow stack. Arguments 1–6 use registers; argument 7 is from the stack.

#### Test 3: test_va_arg_pointer
**Pattern**: Pointer type extraction.

**Source**:
```C
#include <stdarg.h>

int read_ptr(int ignored, ...) {
    va_list ap;
    int *p;
    
    va_start(ap, ignored);
    p = va_arg(ap, int *);
    va_end(ap);
    
    return *p;
}

int main(void) {
    int x = 42;
    return read_ptr(0, &x);
}
```

**Expected return**: `42`

**Significance**: Tests pointer type handling (load 8 bytes as a pointer address).

#### Test 4: test_va_arg_string_strcmp
**Pattern**: Pointer to string type with strcmp verification.

**Source**:
```C
#include <stdarg.h>
#include <string.h>

int check(int ignored, ...) {
    va_list ap;
    const char *s;
    
    va_start(ap, ignored);
    s = va_arg(ap, const char *);
    va_end(ap);
    
    return strcmp(s, "hello") == 0;
}

int main(void) {
    return check(0, "hello");
}
```

**Expected return**: `1` (true)

**Significance**: Tests string pointer extraction and usage with standard library function.

#### Test 5: test_va_arg_long_long
**Pattern**: 64-bit integer extraction.

**Source**:
```C
#include <stdarg.h>

int check(int ignored, ...) {
    va_list ap;
    long long x;
    
    va_start(ap, ignored);
    x = va_arg(ap, long long);
    va_end(ap);
    
    return x == 42LL;
}

int main(void) {
    return check(0, 42LL);
}
```

**Expected return**: `1` (true)

**Significance**: Tests `long long` type extraction and comparison.

#### Test 6: test_va_arg_mixed_types
**Pattern**: Multiple different integer/long types in sequence.

**Source**:
```C
#include <stdarg.h>

int mixed(int ignored, ...) {
    va_list ap;
    int a;
    long b;
    unsigned long long c;
    
    va_start(ap, ignored);
    
    a = va_arg(ap, int);
    b = va_arg(ap, long);
    c = va_arg(ap, unsigned long long);
    
    va_end(ap);
    
    return a + b + c;
}

int main(void) {
    return mixed(0, 10, 20L, 12ULL);
}
```

**Expected return**: `42` (10 + 20 + 12)

**Significance**: Tests correct handling of multiple types in the same va_arg sequence.

### Invalid Tests (2 new)

#### Test 1: test_va_arg_char_reject
**Pattern**: Reject small promoted types (char).

**Source**:
```C
#include <stdarg.h>

int f(int ignored, ...) {
    va_list ap;
    char c;
    
    va_start(ap, ignored);
    c = va_arg(ap, char);
    va_end(ap);
    
    return c;
}

int main(void) {
    return f(0);
}
```

**Expected**: Compilation error during codegen (va_arg does not support `char`).

**Significance**: Validates that the compiler rejects small promoted types, which must be explicitly cast by the user.

#### Test 2: test_va_arg_struct_reject
**Pattern**: Reject struct by value.

**Source**:
```C
#include <stdarg.h>

struct Point {
    int x;
    int y;
};

int f(int ignored, ...) {
    va_list ap;
    struct Point p;
    
    va_start(ap, ignored);
    p = va_arg(ap, struct Point);
    va_end(ap);
    
    return p.x;
}

int main(void) {
    return f(0);
}
```

**Expected**: Compilation error during codegen (va_arg does not support struct by value).

**Significance**: Validates that the compiler rejects aggregate types, which would require custom extraction logic.

## Implementation Order

1. Update `src/codegen.c` — replace `AST_BUILTIN_VA_ARG` handler stub with full algorithm
2. Update `src/version.c` from `"00750500"` to `"00750600"`
3. Create integration test directories and files (6 valid + 2 invalid)

## Ambiguities and Decisions

- **Small type rejection**: The spec notes that char, short, and _Bool must be cast by the user (e.g., `va_arg(ap, int)` instead of `va_arg(ap, char)`). The codegen enforces this at type-check time.
- **Pointer to aggregate**: Pointers to struct/union are allowed (e.g., `struct Token *`) because the pointer itself is 8 bytes GP register class. Only struct/union **by value** is rejected.
- **Load semantics**: For `int` from an 8-byte slot, the value must be sign-extended (via `movsxd eax, dword [src]`). For `unsigned int`, zero-extended (via `mov eax, dword [src]` with implicit zero-extend).
- **Spec issues noted**: The spec contains several test bugs that will be corrected in the actual test files:
  - pick7 main() uses comma expression instead of `return pick7(...)` call
  - long_long test has missing vararg in the call and uses assignment `x = 42LL` instead of comparison `x == 42LL`
  - "small types" list mentions `int` but means `char`
  - struct test missing `#` on `#include`
  - Minor typo: field name is `reg_save_area` (not `reg_save_arga`)
