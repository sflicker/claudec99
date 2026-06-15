# Stage 130 — `va_arg` for Struct-by-Value Types

## Overview

`va_arg(ap, struct T)` currently compiles with:

```
error: va_arg: aggregate types are not supported
```

This stage implements the SysV AMD64 ABI algorithm for extracting a
struct or union value from a variadic argument list.

---

## Background: SysV AMD64 ABI Classification

The ABI classifies struct arguments into two classes:

- **MEMORY class**: size > 16 bytes, or (in a full implementation) any
  eightbyte containing an unaligned field.  Passed on the stack via
  `overflow_arg_area`.
- **Register class (INTEGER)**: size 1–8 bytes → 1 GP eightbyte;
  size 9–16 bytes → 2 GP eightbytes.  Passed in consecutive GP registers.

`struct_reg_eightbytes(Type *st)` in `src/codegen.c` already encodes
this rule (size-based only; structs with FP members are out of scope):

```c
static int struct_reg_eightbytes(Type *st) {
    if (!st) return 0;
    int sz = st->size;
    if (sz <= 0 || sz > 16) return 0;      /* MEMORY class */
    return (sz > 8) ? 2 : 1;               /* 1 or 2 GP eightbytes */
}
```

---

## va_list Layout (recap)

```
[rbp - ap_off]       gp_offset        uint32 (bytes  0–3 of va_list)
[rbp - ap_off + 4]   fp_offset        uint32 (bytes  4–7)
[rbp - ap_off + 8]   overflow_arg_area void * (bytes  8–15)
[rbp - ap_off + 16]  reg_save_area    void * (bytes 16–23)
```

In the existing asm notation used throughout codegen:

| Field              | Address               |
| ------------------ | --------------------- |
| `gp_offset`        | `[rbp - ap_off]`      |
| `fp_offset`        | `[rbp - (ap_off-4)]`  |
| `overflow_arg_area`| `[rbp - (ap_off-8)]`  |
| `reg_save_area`    | `[rbp - (ap_off-16)]` |

---

## Algorithm

### Scratch slot

The result of `va_arg(ap, struct T)` is a struct rvalue.  It must be
materialized into a local stack slot so consumers (assignments, member
access, passing by value) have a pointer to copy from.  The existing
`claim_struct_ret_temp(cg, sz)` mechanism is reused.

`compute_struct_ret_bytes` is extended to account for
`AST_BUILTIN_VA_ARG` nodes whose type is a struct/union.

### Case 1: MEMORY class (`ebs == 0`, size > 16)

Structs larger than 16 bytes are always in `overflow_arg_area`:

```
rdx  ← overflow_arg_area           ; [rbp - (ap_off-8)]
overflow_arg_area ← rdx + align8(size)
copy size bytes from [rdx] → [rbp - temp_off]  (rep movsb)
rax  ← lea [rbp - temp_off]
```

### Case 2: Register class, 1 eightbyte (`ebs == 1`, size 1–8)

```
eax ← gp_offset
if eax >= 48: goto overflow
  rdx ← reg_save_area + eax        ; [rbp - (ap_off-16)]
  gp_offset += 8
  goto load
overflow:
  rdx ← overflow_arg_area
  overflow_arg_area += 8
load:
  mov rax, [rdx]                   ; load 8 bytes (ABI slot always 8 B)
  mov [rbp - temp_off], rax
rax ← lea [rbp - temp_off]
```

### Case 3: Register class, 2 eightbytes (`ebs == 2`, size 9–16)

Both GP slots must be available simultaneously (i.e., `gp_offset ≤ 32`):

```
eax ← gp_offset
if eax > 32: goto overflow          ; need 2 slots: offset ≤ 32
  rcx ← reg_save_area + eax
  r11 ← [rcx],  r10 ← [rcx+8]
  mov [rbp - temp_off],     r11
  mov [rbp - (temp_off-8)], r10
  gp_offset += 16
  goto done
overflow:
  rdx ← overflow_arg_area
  r11 ← [rdx],  r10 ← [rdx+8]
  mov [rbp - temp_off],     r11
  mov [rbp - (temp_off-8)], r10
  overflow_arg_area += align8(size)
done:
rax ← lea [rbp - temp_off]
```

---

## Implementation Sites

### `src/codegen.c` — `compute_struct_ret_bytes`

Add handling for `AST_BUILTIN_VA_ARG`:

```c
if (node->type == AST_BUILTIN_VA_ARG) {
    TypeKind vk = node->full_type ? node->full_type->kind
                                  : (TypeKind)node->decl_type;
    if (is_struct_or_union_kind(vk) && node->full_type && node->full_type->size > 0) {
        int sz = node->full_type->size;
        total += ((sz + 7) & ~7) + 8;
    }
}
```

### `src/codegen.c` — `AST_BUILTIN_VA_ARG` handler

Replace:

```c
case TYPE_STRUCT:
case TYPE_UNION:
    compile_error("error: va_arg: aggregate types are not supported\n");
    return;
```

with the three-case implementation described above.

---

## Tests

### `test/valid/variadic/test_va_arg_struct_memory__0.c`

```c
#include <stdarg.h>
#include <string.h>

typedef struct { long a; long b; long c; } Big;   /* 24 bytes — MEMORY class */

static Big extract(int n, ...) {
    va_list ap;
    va_start(ap, n);
    Big s = va_arg(ap, Big);
    va_end(ap);
    return s;
}

int main(void) {
    Big b = {10, 20, 30};
    Big r = extract(1, b);
    if (r.a == 10 && r.b == 20 && r.c == 30) return 0;
    return 1;
}
```

### `test/valid/variadic/test_va_arg_struct_reg1__0.c`

```c
#include <stdarg.h>

typedef struct { int x; int y; } Point;   /* 8 bytes — 1 GP eightbyte */

static Point first(int n, ...) {
    va_list ap;
    va_start(ap, n);
    Point p = va_arg(ap, Point);
    va_end(ap);
    return p;
}

int main(void) {
    Point p = {3, 7};
    Point r = first(1, p);
    return (r.x == 3 && r.y == 7) ? 0 : 1;
}
```

### `test/valid/variadic/test_va_arg_struct_reg2__0.c`

```c
#include <stdarg.h>

typedef struct { long a; long b; } Pair;  /* 16 bytes — 2 GP eightbytes */

static Pair grab(int n, ...) {
    va_list ap;
    va_start(ap, n);
    Pair s = va_arg(ap, Pair);
    va_end(ap);
    return s;
}

int main(void) {
    Pair p = {100, 200};
    Pair r = grab(1, p);
    return (r.a == 100 && r.b == 200) ? 0 : 1;
}
```

---

## Acceptance Criteria

- All three test programs compile and exit 0.
- Existing test suite passes (0 failures, no regressions).
- Version bumped to `01300000`.
