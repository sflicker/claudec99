# Stage 34 Kickoff: Struct Pointer Parameters and Mutation Tests

## Spec Summary

Stage 34 adds support for passing pointers to structs as function parameters. Functions may receive `struct T *p` parameters and use the `->` operator for field access and mutation. Callers can pass `&local_struct` or `&global_struct`. The `(*p).field` deref-dot syntax should also work.

### Example Goals
```c
struct Point {
    int x;
    int y;
};

int move(struct Point *p, int dx, int dy) {
    p->x = dx;
    p->y = dy;
    return 0;
}

int main() {
    struct Point p;
    p.x = 10;
    p.y = 20;
    move(&p, 3, 4);
    return p.x + p.y;  // expect 7
}
```

## Spec Issues Found

**FLAG: Spec has 5 errors that must be fixed before implementation:**

1. **Line 14**: `Struct Point *p` → should be `struct Point *p` (capital S typo)
2. **Line 25**: `// expect 37` → should be `// expect 7` (after move(3,4): p.x=3, p.y=4, sum=7)
3. **Line 82**: `return p->x + p->x` → should be `p->x + p->y` to correctly yield 33
4. **Line 108**: `int(&c)` → syntax error, should be `inc(&c)` (second increment call)
5. **Invalid test "P is a struct pointer not struct object"** (lines 142-154): The test shows passing `&p` to a function expecting `struct Point` (by value), but the INVALID comment is on `p.x = dx`. The actual invalid case is at the call site passing a pointer to a by-value parameter. The comment placement is misleading.

## Tokenizer Changes Required

**None** — `TOKEN_ARROW` already exists from stage 31.

## Parser Changes Required

**None** — `parse_postfix` already handles `->` operator and `(*p).field` parses correctly (AST_DEREF as child of MEMBER_ACCESS).

## AST Changes Required

**None** — `AST_ARROW_ACCESS` and `AST_MEMBER_ACCESS` already exist from stage 31.

## Codegen Changes Required

The codegen must extend member access handling to support dereferencing pointers:

1. **`emit_member_addr`** (src/codegen.c ~line 534)
   - Extend to handle `AST_DEREF` as the base expression
   - When base is a dereferenced pointer, load the pointer value, add the member offset, and emit the address

2. **`expr_result_type`** (src/codegen.c ~line 922)
   - Extend the `MEMBER_ACCESS` case to handle `AST_DEREF` base
   - Return the field type (same as direct field access)

3. **`sizeof_type_of_expr`** (src/codegen.c ~line 767)
   - Extend the `MEMBER_ACCESS` case to handle `AST_DEREF` base
   - Return the size of the field type

## Test Plan

### Valid Cases (should compile and execute)
- **test_struct_ptr_param_mutate__7.c**: Local struct pointer parameter, mutation via `->`, verify mutations persist
  - `move(&p, 3, 4)` then return `p.x + p.y` = 7
- **test_struct_ptr_param_read__33.c**: Read struct pointer parameter via `->`
  - `sum_point(&p)` returns `p->x + p->y` = 33
- **test_struct_ptr_param_inc__42.c**: Multiple mutations on same struct via pointer
  - Two `inc(&c)` calls, final `c.value` = 42

### Invalid Cases (should fail at compile time)
- **test_struct_ptr_missing_field**: Accessing non-existent field via `->` (p->z on struct Point)
- **test_struct_ptr_arrow_on_obj**: Using `->` on struct object instead of pointer (p->x where p is struct object)
- **test_struct_ptr_for_byval_param**: Passing `&p` to function expecting struct by value (type mismatch)

## Implementation Order

1. Extend `emit_member_addr` to handle `AST_DEREF` base
2. Extend `expr_result_type` for MEMBER_ACCESS with AST_DEREF base
3. Extend `sizeof_type_of_expr` for MEMBER_ACCESS with AST_DEREF base
4. Add valid test cases
5. Add invalid test cases
6. Run test suite and verify all tests pass
7. Commit changes

## Ambiguities and Decisions

- **Deref-dot syntax `(*p).field`**: Already supported by existing parser (produces AST_DEREF with MEMBER_ACCESS child). No changes needed for this to work.
- **Global vs local struct pointers**: Both `&local_struct` and `&global_struct` should work via the address-of operator. No special handling needed.
- **Mutation semantics**: Changes via `->` on a parameter pointer must persist because the parameter is a pointer (pass-by-reference semantics for the struct). This differs from by-value struct parameters.
