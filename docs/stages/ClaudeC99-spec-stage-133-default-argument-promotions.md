# ClaudeC99 Stage 133 - Default Argument Promotions In Function Calls

## Goal

Implement correct C99 default argument promotion behavior in cc99 for function
calls where the callee type does not provide parameter type information, and
preserve the already-working behavior for variadic arguments after the fixed
parameters.

Default argument promotions are required by C99 for:

- trailing arguments passed through `...` in a variadic function call
- every argument in a call to a function declared without a prototype

The promotions are:

- integer promotions for narrow integer types such as `char`, `signed char`,
  `unsigned char`, `short`, and `unsigned short`
- `float` promoted to `double`

## Test Results

Tested on 2026-06-16 with:

```text
/home/scott/code/claude/claudec99/bin/cc99
gcc -std=c99 -O0 -Wall -Wextra
clang -std=c99 -O0 -Wall -Wextra
```

### Variadic Arguments

Reduced test:

```c
#include <stdarg.h>

int check(int count, ...) {
    va_list ap;
    int c;
    double f;

    va_start(ap, count);
    c = va_arg(ap, int);
    f = va_arg(ap, double);
    va_end(ap);

    return c == 65 && (int)(f * 10.0) == 15;
}

int main(void) {
    char c = 65;
    float f = 1.5f;

    return check(2, c, f);
}
```

Expanded regression file:

```text
cc99_testing/test_default_argument_promotions.c
```

Observed results:

- GCC returns `127`
- Clang returns `127`
- cc99 returns `127`

cc99 already handles default argument promotions for this variadic call path.

### Calls Without A Prototype

Reduced test:

```c
int check();

int check(int c, int s, int us, double f, double d) {
    int score = 0;

    if (c == 65) {
        score = score + 1;
    }

    if (s == -1234) {
        score = score + 2;
    }

    if (us == 60000) {
        score = score + 4;
    }

    if ((int)(f * 10.0) == 15) {
        score = score + 8;
    }

    if ((int)(d * 10.0) == 25) {
        score = score + 16;
    }

    return score;
}

int main(void) {
    char c = 65;
    short s = -1234;
    unsigned short us = 60000;
    float f = 1.5f;
    double d = 2.5;

    return check(c, s, us, f, d);
}
```

Expanded regression file:

```text
cc99_testing/test_default_promotions_no_prototype.c
```

Expected result:

```text
31
```

Observed production compiler behavior:

- GCC compiles and returns `31`
- Clang compiles with a deprecation warning for the non-prototype declaration
  and returns `31`

Observed cc99 behavior:

```text
/home/scott/code/c/cc99_benchmarks_and_testing/cc99_testing/test_default_promotions_no_prototype.c:8:53: error: function 'check' parameter count mismatch (0 vs 5)
```

cc99 appears to treat `int check();` as a prototype with zero parameters. In
C99, a declaration with an empty parameter list is not a prototype. It declares
a function with no parameter type information.

## Required Behavior

For a declaration such as:

```c
int check();
```

cc99 must represent the function type as having no prototype information, not as
having a fixed zero-parameter prototype.

Calls through that type must apply default argument promotions to every
argument:

```c
char           -> int
signed char    -> int
unsigned char  -> int, if all values fit in int on the target
short          -> int
unsigned short -> int, if all values fit in int on the target
float          -> double
```

On the current SysV AMD64 target, `unsigned short` values fit in `int`, so
`unsigned short` promotes to `int`.

The resulting call must use the promoted argument types for semantic analysis
and ABI lowering. For example:

```c
char c = 65;
float f = 1.5f;
check(c, f);
```

must be lowered as if the call arguments had type:

```c
int, double
```

## Function Declaration And Definition Compatibility

cc99 should allow a later prototype definition to match an earlier non-prototype
declaration when the return types are compatible:

```c
int check();

int check(int c, double f) {
    return c + (int)f;
}
```

The earlier declaration does not specify zero parameters, so the later
definition must not fail with a parameter count mismatch.

If an old-style function definition is supported later, the same default
argument promotion rules must be considered when checking the definition's
parameter declarations. This report only requires support for calls through a
non-prototype declaration followed by a prototype definition.

## Semantic Analysis Requirements

1. Distinguish these function type shapes:
   - prototype with zero parameters: `int f(void);`
   - no prototype information: `int f();`
   - prototype with parameters: `int f(int, double);`
2. For a call where the callee has no prototype information, apply default
   argument promotions to each argument before call type checking and lowering.
3. For a variadic call where fixed parameters are known, apply normal assignment
   conversion to fixed arguments and default argument promotions only to trailing
   variadic arguments.
4. Preserve existing diagnostics for genuinely incompatible function
   redeclarations.
5. Keep the comparison result and expression value behavior independent of
   whether the promoted values are passed through registers or stack slots.

## Code Generation Requirements

The call-lowering path must receive promoted argument types when the callee has
no prototype:

- integer-promoted values should be extended to the promoted integer type before
  assignment to integer argument registers or stack slots
- `float` values should be converted to `double` and passed using the target
  ABI's `double` classification
- `double` values remain `double`

This is especially important on SysV AMD64 because `float` and `double` have
different storage widths even though both use SSE registers.

## Regression Tests

### Variadic Promotions

Save as `test_default_argument_promotions__127.c`.

Expected return code:

```text
127
```

This test should continue to pass under cc99, GCC, and Clang.

### No-Prototype Call Promotions

Save as `test_default_promotions_no_prototype__31.c`.

Expected return code:

```text
31
```

This test currently fails to compile under cc99 and should pass after
implementation.

## Acceptance Criteria

1. `test_default_argument_promotions__127.c` compiles and returns `127` under cc99,
   GCC, and Clang.
2. `test_default_promotions_no_prototype__31.c` compiles and returns `31` under
   cc99, GCC, and Clang.
3. `int f(void);` remains a true zero-parameter prototype.
4. `int f();` is represented as a declaration without prototype information and
   is compatible with a later definition such as `int f(int, double)`.
