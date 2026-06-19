# ClaudeC99 Stage 140 pointer-size typedef behavior

This report covers C99 pointer-size typedef behavior tested with:

```text
/home/scott/code/claude/claudec99/bin/cc99
```

The reduced example was compared against GCC using
`gcc -std=c99 -O0 -Wall -Wextra -pedantic`. GCC accepted the program and
returned the expected value.

cc99 was compiled with its bundled include directory:

```text
/home/scott/code/claude/claudec99/test/include
```

## Summary

| ID | Area | Symptom | Priority |
|----|------|---------|----------|
| CC99-013 | Type system / unsigned typedef arithmetic | `size_t` arithmetic behaves as signed after casts to `size_t` | Medium |

## CC99-013: Casts To Size_t Do Not Preserve Unsigned Arithmetic Semantics

### Reduced Source

```c
#include <stddef.h>
#include <stdint.h>

int main(void) {
    int values[5] = { 1, 2, 3, 4, 5 };
    int *first = &values[0];
    int *last = &values[4];
    size_t object_size = sizeof values;
    size_t element_size = sizeof values[0];
    ptrdiff_t forward = last - first;
    ptrdiff_t backward = first - last;
    intptr_t address = (intptr_t)last;
    int *again = (int *)address;

    if (object_size / element_size != 5) {
        return 101;
    }
    if (sizeof(size_t) != sizeof(void *)) {
        return 102;
    }
    if (sizeof(ptrdiff_t) != sizeof(void *)) {
        return 103;
    }
    if (sizeof(intptr_t) != sizeof(void *)) {
        return 104;
    }
    if (!((size_t)0 - (size_t)1 > 0)) {
        return 105;
    }
    if (forward != 4) {
        return 106;
    }
    if (backward != -4) {
        return 107;
    }
    if (sizeof(forward) != sizeof(ptrdiff_t)) {
        return 108;
    }
    if (again != last) {
        return 109;
    }
    if (*again != 5) {
        return 110;
    }

    return 10;
}
```

### Compile Commands

```sh
/home/scott/code/claude/claudec99/bin/cc99 -I /home/scott/code/claude/claudec99/test/include -o /tmp/test_std_pointer_size_typedefs_cc99 cc99_testing/test_std_pointer_size_typedefs.c
gcc -std=c99 -O0 -Wall -Wextra -pedantic -o /tmp/test_std_pointer_size_typedefs_gcc cc99_testing/test_std_pointer_size_typedefs.c
```

### Observed Behavior

GCC accepts the program and returns `10`.

cc99 accepts the program but returns `105`, which identifies this failing
check:

```c
((size_t)0 - (size_t)1 > 0)
```

An earlier aggregate version of the same test returned `9` instead of `10`,
which is consistent with only this check failing.

### Expected Behavior

`stddef.h` defines `size_t` as an unsigned integer type. On the current cc99
LP64 target, the bundled header uses:

```c
typedef unsigned long size_t;
```

The expression `(size_t)0 - (size_t)1` should therefore wrap to the maximum
representable `size_t` value, and the comparison with `0` should be true. The
reduced program should return `10`.

The same reduced test also verifies that:

- `size_t`, `ptrdiff_t`, and `intptr_t` are visible through `<stddef.h>` and
  `<stdint.h>`
- their sizes match pointer width on this target
- pointer subtraction produces usable signed `ptrdiff_t` values
- pointer-to-`intptr_t` and back round-trips for the tested pointer value

Those checks passed before the failing `size_t` arithmetic check was reached,
and the aggregate score indicates the later pointer-difference and pointer
round-trip checks also behaved as expected in this reduced case.

### Likely Fix Area

The cast or typedef type path appears to preserve the width of `size_t` but not
its unsignedness for arithmetic and comparison. Ensure that typedef aliases
carry the signedness of their underlying integer type through:

- cast expression result typing, especially `(size_t)expr`
- usual arithmetic conversions for binary `-`
- relational comparison typing for `>`
- constant folding paths for unsigned typedef operands

This overlaps with the existing `sizeof`/`size_t` typing concern, but this
reduced case shows the same issue for explicit casts to the `size_t` typedef.

### Regression Tests To Add

- `(size_t)0 - (size_t)1 > 0`
- `sizeof(int) > -1` and `sizeof(char) - 2 > 0`
- arithmetic through a `size_t` local variable
- pointer subtraction assigned to `ptrdiff_t`, including negative differences
- pointer round-trip through `intptr_t`
