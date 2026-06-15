# ClaudeC99 Stage 131 - Specification: `sizeof` Must Produce Unsigned `size_t`

## Goal

Implement correct typing and conversion behavior for the `sizeof` operator
in ClaudeC99.

In C99 compliant compiler, the result of `sizeof` has type `size_t`, which is an unsigned integer
type defined by the implementation. ClaudeC99 currently appears to treat `sizeof` as
a signed integer in at least some expression-lowering paths.

## Required Behavior

`sizeof expression` and `sizeof(type-name)` must produce a value whose type is
`size_t`.

That means `sizeof` must participate in:

- integer promotions
- usual arithmetic conversions
- relational comparisons
- equality comparisons
- constant expression evaluation
- static initializers
- array bounds
- return expressions

as an unsigned integer type, not as signed `int`.

## Sample Regression Program

Save as `test_sizeof_size_t__6.c`:

```c
/*
 * sizeof result type test.
 *
 * In C99, sizeof yields size_t, an unsigned integer type. These checks detect
 * compilers that incorrectly lower sizeof as a signed int.
 *
 * Expected conforming return code: 6.
 */
int main(void) {
    int score = 0;

    if (sizeof(int) > -1) {
        score = score + 1;
    }

    if (sizeof(char) - 2 > 0) {
        score = score + 2;
    }

    if ((sizeof(int) < 0) == 0) {
        score = score + 4;
    }

    return score;
}
```

Expected result from a conforming C99 compiler:

```text
6
```

Observed cc99 (ClaudeC99) result on 2026-06-15:

```text
5
```

## Why The Expected Result Is `6`

### Check 1

```c
sizeof(int) > -1
```

Because `sizeof(int)` has unsigned type `size_t`, the `-1` operand is converted
to `size_t` for the comparison. That produces the maximum representable
`size_t` value, so the comparison is false.

This check must not add `1`.

### Check 2

```c
sizeof(char) - 2 > 0
```

`sizeof(char)` is `1`, with unsigned type. Subtracting `2` under unsigned
arithmetic wraps to a large unsigned value, so the comparison is true.

This check must add `2`.

### Check 3

```c
(sizeof(int) < 0) == 0
```

An unsigned `sizeof(int)` value cannot be less than zero after usual arithmetic
conversions, so `sizeof(int) < 0` is false.

This check must add `4`.

Total expected score: `0 + 2 + 4 = 6`.

cc99 returning `5` is consistent with:

- `sizeof(int) > -1` incorrectly evaluating true under signed comparison
- `sizeof(char) - 2 > 0` incorrectly evaluating false under signed arithmetic
- `sizeof(int) < 0` still evaluating false

## Implementation Requirements

### Type Representation

cc99 (ClaudeC99) must have a canonical `size_t` type available to semantic analysis and
code generation.

On the current SysV AMD64 target, `size_t` should normally be equivalent to:

```c
unsigned long
```

The implementation should avoid hard-coding this in expression-specific logic
if cc99 (ClaudeC99) already has a target type table or builtin type registry.

### Semantic Analysis

When parsing or analyzing either form:

```c
sizeof expression
sizeof(type-name)
```

the expression node should be assigned:

- value category: non-lvalue
- type: canonical `size_t`
- value: compile-time byte size of the operand type

For `sizeof expression`, the operand must not be evaluated at runtime except for
C99 variable length array cases if cc99 (ClaudeC99) supports VLAs. If cc99 (ClaudeC99( does not support
VLAs, keep existing VLA diagnostics unchanged.

### Constant Expression Evaluation

The constant evaluator must preserve the unsigned type of `sizeof`, not just
the numeric value.

For example, this expression must be evaluated using unsigned conversion rules:

```c
sizeof(char) - 2
```

It is not enough to store only the integer value `1`; the evaluator must also
know that the value has unsigned `size_t` type.

### Usual Arithmetic Conversions

Expression typing for binary operators must treat `size_t` as unsigned.

The following operators are especially relevant:

- `+`
- `-`
- `*`
- `/`
- `%`
- `<`
- `<=`
- `>`
- `>=`
- `==`
- `!=`
- `&&`
- `||`

For comparisons between `size_t` and signed integer operands, cc99 (ClaudeC99( should follow
C99 usual arithmetic conversions.

### Code Generation

Generated code for `sizeof` values should use the storage width and signedness
of `size_t`.

For SysV AMD64:

- materialize `sizeof` constants as 64-bit unsigned integer values when needed
- use unsigned comparison instructions or condition selection where the usual
arithmetic conversions require unsigned comparison
- preserve unsigned arithmetic behavior for subtraction and comparison

## Suggested Regression Tests

Add tests for both `sizeof expression` and `sizeof(type-name)`:

```c
sizeof(int) > -1
sizeof(char) - 2 > 0
sizeof(int) < 0
sizeof x > -1
sizeof x - 2 > 0
```

Add tests in these contexts:

- ordinary runtime expressions
- `if` conditions
- return expressions
- integer constant expressions
- enum constants if supported
- file-scope static initializers
- array bounds

## Acceptance Criteria

The sample program in this document must:

1. compile successfully with cc99 (ClaudeC99)
2. run without a signal
3. return exit code `6`

The same behavior should match GCC and Clang in C99 mode.
