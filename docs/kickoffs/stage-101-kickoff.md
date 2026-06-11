# Stage 101 Kickoff — Block-Scope Static Arrays and Structs

## Summary

Stage 101 extends block-scope `static` local variable support from scalars to aggregates. The guard rejecting `TYPE_ARRAY`, `TYPE_STRUCT`, and `TYPE_UNION` at declaration time is removed, and codegen is extended to emit initialized and uninitialized arrays and structs to `.data` and `.bss` respectively, using RIP-relative addressing via compiler-generated private labels.

After this stage, patterns like:

```c
void histogram(int bucket) {
    static int counts[8];     /* uninitialized — zero in .bss */
    static int totals[4] = {0, 0, 0, 0};  /* initialized — .data */
    counts[bucket]++;
    totals[bucket % 4]++;
}

void origin_demo() {
    static struct Point org = {1, 2};
    org.x++;
}
```

will compile and run correctly, with the static arrays and structs persisting across calls.

---

## Required Tokenizer Changes

None. No new tokens required.

---

## Required Parser Changes

None. The parser already handles all `static` local declarations.

---

## Required AST Changes

None. Existing `AST_INITIALIZER_LIST` and `AST_STRING_LITERAL` nodes suffice for array and struct initializers.

---

## Required Code Generation Changes

**Task 1: Extend `LocalStaticVar` struct in `include/codegen.h`**

Add an `ASTNode *init_node` field to hold the brace-initializer for arrays and structs. This field is `NULL` for scalars and for uninitialized entries.

**Task 2: Remove the guard and add array/struct static registration in `codegen_statement` SC_STATIC arm**

Delete the guard rejecting `TYPE_ARRAY`, `TYPE_STRUCT`, `TYPE_UNION`. Add registration logic for static array locals and static struct/union locals, mirroring the existing scalar path but storing the initializer node in `LocalStaticVar`.

**Task 3: Extend `codegen_emit_local_statics` for arrays and structs**

- For initialized arrays: emit char arrays initialized from string literals as `db` directives; emit other arrays using the slots-map pattern from global array emit.
- For initialized structs: call `emit_global_struct`.
- For initialized unions: inline the union first-member logic from global union emit.
- For uninitialized arrays, structs, and unions: emit appropriate `resb` or typed `res*` directives.

**Task 4: Fix array decay in `codegen_expression` VAR_REF branch**

When a static array is used as a pointer (array decay), emit `lea rax, [rel label]` instead of stack-relative addressing.

**Task 5: Fix array subscript base in `emit_array_index_addr`**

Add `is_static` branch to emit RIP-relative addressing for static arrays used as subscript base.

**Task 6: Fix struct member address in `emit_member_addr`**

For static structs, emit RIP-relative addressing with offset (`[rel label + offset]` or `[rel label]`).

---

## Test Plan

**Valid tests** (6):
1. Uninitialized static int array persists across calls
2. Initialized static int array with brace-list
3. Uninitialized static struct with member updates
4. Initialized static struct with brace-list
5. Static char array initialized from string literal
6. Static array persistence and correct indexing

**Invalid tests** (2):
1. Non-constant initializer (variable reference) for static array
2. Non-constant initializer (variable reference) for static struct

**Print-AST tests**: None required.

---

## Implementation Order

1. Add `ASTNode *init_node` to `LocalStaticVar` in `include/codegen.h` (Task 1)
2. Remove guard and add array/struct registration in `codegen_statement` SC_STATIC arm (Task 2)
3. Extend `codegen_emit_local_statics` for array/struct emission (Task 3)
4. Fix array decay in `codegen_expression` VAR_REF branch (Task 4)
5. Fix array subscript base in `emit_array_index_addr` (Task 5)
6. Fix struct member address in `emit_member_addr` (Task 6)
7. Bump `VERSION_STAGE` to `"01010000"` in `src/version.c`
8. Add tests
9. Update documentation (after all tests pass)

---

## Notes

- This is a codegen-only stage; no tokenizer, parser, or AST changes.
- Block-scope scalar `static` locals have been supported since stage 71; this stage extends to aggregates.
- Out of scope: designated initializers, multidimensional arrays, static arrays of structs, runtime initializers, compound literals, floating-point members.
- Bootstrap verification: run `./build.sh --mode=self-host` before declaring the stage complete.
- No ambiguities found in the spec.
