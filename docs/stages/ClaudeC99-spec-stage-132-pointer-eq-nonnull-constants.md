# ClaudeC99 Stage 131 - Pointer Equality With Non-Null Constants

## Goal

Implement cc99 support for equality comparisons between pointer operands and
non-zero integer constant operands, matching the extension behavior accepted by
GCC and Clang.

This feature is separate from strictly conforming C99 pointer equality. C99
requires pointer/integer equality comparisons to use a null pointer constant
when the non-pointer operand is an integer expression. GCC and Clang accept
non-zero integer constants as an extension, diagnose the comparison, and compare
the pointer value against the integer constant converted to pointer-width form.

## Current cc99 Behavior

Reduced program:

```c
int main(void) {
    int *one = (int *)1;

    if (one == 1) {
        return 1;
    }

    return 0;
}
```

Observed on 2026-06-15:

```text
test_pointer_eq_integer_constants.c:16:20: error: comparing pointer with non zero integer
```

GCC and Clang compile the same form in C99 mode with pointer/integer comparison
warnings.

## Required Behavior

For `==` and `!=`, cc99 should accept an expression where:

- one operand has pointer type
- the other operand is an integer constant expression
- the integer constant expression value is non-zero

The comparison result must have type `int` and value `0` or `1`, as for other C
equality comparisons.

The integer constant must be converted to the pointer's representation width for
comparison. On the current SysV AMD64 target, that means comparing the pointer
value against a 64-bit address value derived from the integer constant.

Examples that should compile and run:

```c
int *one = (int *)1;
int *two = (int *)2;

one == 1
one != 2
two == 2
two != 1
```

The implementation may still emit a warning or extension diagnostic similar to
GCC/Clang:

```text
warning: comparison between pointer and integer
```

It must not reject the program solely because the integer constant is non-zero.

## Standard C99 Behavior To Preserve

Strictly conforming pointer equality must continue to work:

```c
int *p = 0;
p == 0
p != 0
p == (int *)1
p != (int *)2
```

Pointer-to-pointer equality with explicit casts to non-null constants is already
valid C as a comparison between pointer operands. cc99 currently accepts this
form and should keep doing so.

The extension should not silently permit unrelated operators. These remain
outside the scope of this feature unless separately implemented:

- pointer relational comparisons against non-zero integers, such as `p < 1`
- pointer arithmetic with integer constants beyond normal pointer arithmetic
- implicit pointer initialization from non-zero integers, such as `int *p = 1`

## Semantic Analysis Requirements

When analyzing equality operators:

1. Detect pointer/integer pairs for `==` and `!=`.
2. If the integer operand is an integer constant expression:
   - allow value `0` through the existing null pointer constant path
   - allow non-zero values through this extension path
3. Preserve or emit an extension warning for non-zero pointer/integer equality.
4. Assign the comparison expression type `int`.
5. Record enough conversion information for code generation and constant
   evaluation to compare pointer-width values.

Non-constant integer expressions do not need to be accepted for this feature.
If cc99 chooses to match GCC/Clang more broadly later, that should be a separate
decision.

## Constant Evaluation Requirements

If both operands can be folded, the evaluator should compare the integer address
values after pointer-width conversion:

```c
(int *)1 == 1   /* true under this extension */
(int *)1 != 2   /* true under this extension */
```

The evaluator must still distinguish null pointer constants from ordinary
non-zero integer constants for diagnostics and C99 conformance mode decisions.

## Code Generation Requirements

For runtime equality against a non-zero integer constant:

- evaluate the pointer operand normally
- materialize the integer constant as a pointer-width immediate
- compare using pointer/integer register width, 64-bit on SysV AMD64
- produce `0` or `1` according to `==` or `!=`

The comparison is an address-value comparison. It must not dereference either
operand.

## Regression Tests

### Extension Test

Save as `test_pointer_eq_integer_constants__15.c`:

```c
/*
 * Expected extension return code: 15.
 */
int main(void) {
    int *one = (int *)1;
    int *two = (int *)2;
    int score = 0;

    if (one == 1) {
        score = score + 1;
    }

    if (one != 2) {
        score = score + 2;
    }

    if (two == 2) {
        score = score + 4;
    }

    if (two != 1) {
        score = score + 8;
    }

    return score;    // expected 15
}
```

Expected exit code:

```text
15
```

Observed production compiler behavior on 2026-06-15:

- GCC: compiles with pointer/integer comparison warnings and returns `15`
- Clang: compiles with pointer/integer comparison warnings and returns `15`

Observed cc99 behavior on 2026-06-15:

- compile failure: `error: comparing pointer with non zero integer`

### Strict Pointer Operand Control Test

Save as `test_pointer_eq_nonnull_constants__63.c`:

```c
/*
 * Expected conforming return code: 63.
 */
int main(void) {
    int local = 0;
    int *local_ptr = &local;
    int *one = (int *)1;
    void *two = (void *)2;
    int score = 0;

    if (one == (int *)1) {
        score = score + 1;
    }

    if (one != (int *)2) {
        score = score + 2;
    }

    if ((int *)0 != one) {
        score = score + 4;
    }

    if (two == (void *)2) {
        score = score + 8;
    }

    if ((char *)3 != (char *)4) {
        score = score + 16;
    }

    if (local_ptr != (int *)1) {
        score = score + 32;
    }

    return score;    // expect 63
}
```

Expected result:

```text
63
```

Observed on 2026-06-15:

- GCC returns `63`
- Clang returns `63`
- cc99 returns `63`

## Acceptance Criteria

1. `test_pointer_eq_integer_constants.c` compiles with cc99.
2. The cc99-built binary returns `15`.
3. The same test continues to return `15` under GCC and Clang in C99 mode.
4. `test_pointer_eq_nonnull_constants.c` continues to compile and return `63`
   under cc99, GCC, and Clang.
