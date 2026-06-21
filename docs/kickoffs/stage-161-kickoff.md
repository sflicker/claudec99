# Stage 161 Kickoff — Void Pointer Compatibility in Comparisons with `--sysinclude`

## Summary of the spec

Stage 161 fixes a type-checking regression when using system headers (`--sysinclude` flag). When comparing `FILE *` to `NULL` with system `stddef.h` (where `NULL` is defined as `((void *)0)`), the compiler incorrectly rejects the comparison as "incompatible pointer types."

**The issue:**

```c
#include <stdio.h>

int main() {
    FILE *fp = NULL;
    if (fp == NULL) {  // ERROR: incompatible pointer types in comparison
        printf("FP is NULL\n");
    }
}
```

Compiling with `cc99 --sysinclude null_test.c` fails with an error at the comparison. The same code:
- Works without `--sysinclude` (stub `stddef.h` defines `NULL` as `0`)
- Works when explicitly cast: `(FILE*)NULL`
- Works with GCC (which allows `void *` to compare with any object pointer)

**Root cause:**

- GCC's system `stddef.h` defines `NULL` as `((void *)0)`, making `fp == NULL` equivalent to `FILE * == void *`.
- The pointer comparison validation in `src/codegen.c` around line 4808 uses `pointer_types_equal()`, which requires exact pointer type equality.
- C99 §6.5.9 permits `void *` to be compared with any object pointer type, but our code does not implement this exception.

**The fix:**

Replace `pointer_types_equal()` with `pointer_types_assignable()` at the two-pointer comparison check in `src/codegen.c` (line ~4808). The `pointer_types_assignable()` function already correctly handles `void *` compatibility with other pointer types per C99 assignment rules.

## Required tokenizer changes

None.

## Required parser changes

None.

## Required AST changes

None.

## Required code generation changes

**File: `src/codegen.c`**

Locate the pointer comparison validation logic around line 4808. The current code uses:

```c
if (!pointer_types_equal(lhs->full_type, rhs->full_type)) {
    CODEGEN_ERROR(...);
}
```

Change `pointer_types_equal` to `pointer_types_assignable` at both pointer comparison checks (when both operands are pointers):

```c
if (!pointer_types_assignable(lhs->full_type, rhs->full_type) &&
    !pointer_types_assignable(rhs->full_type, lhs->full_type)) {
    CODEGEN_ERROR(...);
}
```

This allows `void *` to compare with any object pointer type, matching C99 semantics and GCC behavior.

**File: `src/version.c`**

Update `VERSION_STAGE` from `"01600000"` (Stage 160) to `"01610000"` (Stage 161).

## Test plan

### 1. Portable regression test: void pointer comparisons

**`test/integration/test_void_ptr_cmp/`**

A portable test that exercises void pointer comparisons without requiring system headers:

- Declares a custom struct type
- Creates an untyped pointer via cast: `((struct Box *)NULL)`
- Compares typed pointers to void pointers in multiple contexts (if conditions, assignments)
- Verifies comparisons compile without error
- Expected exit status: 42

This test exercises the bug scenario without dependency on system headers, ensuring the fix works portably.

### 2. System header integration test: FILE pointer comparison

**`test/integration/test_null_file_cmp/`**

An integration test that uses real system headers:

- Includes `<stdio.h>` and `<stdlib.h>`
- Declares a `FILE *` variable
- Compares to `NULL` in conditional contexts
- Verifies the comparison compiles successfully with `--sysinclude`
- Expected exit status: 42

With stub headers (where `NULL=0`), this test would pass before the fix; with system headers (where `NULL=(void*)0`), it requires the fix to pass.

### 3. Regression testing

- Full test suite (`./test/run_all_tests.sh`) must pass
- All existing code generation tests unaffected
- Stage 160 tests (sizeof in constant expressions) continue to pass
- Self-hosting: C0→C1→C2 bootstrap remains valid

## Implementation order

1. Fix `src/codegen.c` — replace `pointer_types_equal` with `pointer_types_assignable` at the two-pointer comparison check
2. Update `src/version.c` — change `VERSION_STAGE` to `"01610000"`
3. Create `test/integration/test_void_ptr_cmp/` — portable test exercising void pointer comparisons
4. Create `test/integration/test_null_file_cmp/` — system header integration test with `FILE * == NULL`
5. Run full test suite and verify C0→C1→C2 self-hosting
6. Commit with Stage 161 reference and Co-Authored-By trailer

## Ambiguities and decisions

- **Decision:** Use `pointer_types_assignable()` bidirectionally rather than introducing a new permissive check. This leverages existing C99-compliant assignment rules already in the codebase and avoids duplicating void pointer compatibility logic.
- **Decision:** Create two integration tests — one portable (void pointer comparisons without system headers) and one that uses real system headers (FILE pointer to NULL). This provides coverage both for the general fix and for the original reported scenario.
- **No spec ambiguities.** The spec clearly identifies the root cause and the intended fix.
