# Stage 100 Kickoff — File-Scope Constant Expressions

## Summary

Stage 100 extends file-scope integer-typed variable initializers from literal-only to full constant expressions. It wires the existing `eval_const_expr` evaluator (already used by `case` labels, enumerators, and array designators) into the global variable initializer path. It also extends `eval_const_primary` with `sizeof(type-name)` support, the only missing operator in the evaluator commonly used in file-scope contexts.

After this stage, patterns like:

```c
int PAGE_SIZE = 1 << 12;              /* shift     */
int MASK      = FLAG_A | FLAG_B;      /* bitwise   */
int BUF_SZ    = sizeof(int) * 1024;  /* sizeof    */
int NEG       = -(3 * 4);            /* arithmetic */
```

will compile at global scope instead of being rejected with `"error: non-constant initializer for global"`.

---

## Required Tokenizer Changes

None. No new tokens required.

---

## Required Parser Changes

**Task 1: Extend `eval_const_primary` with `sizeof(type-name)` support**

In `src/parser.c`, in the `eval_const_primary` function, add handling for `TOKEN_SIZEOF` before the fall-through error at the end. The logic mirrors the sizeof dispatch in `parse_primary` (lines ~1690–1740):

- Consume `sizeof`
- Expect a parenthesized type-name (error if not)
- Parse the type-name and validate it is not an incomplete array type
- Return `(long)type_size(t)`
- Include `TOKEN_ENUM` in the type-start condition (a pre-existing gap in `parse_primary`'s sizeof arm; `sizeof(enum Color)` is commonly written)

**Task 2: Wire `eval_const_expr` to the first-declarator file-scope path**

In `src/parser.c`, inside `parse_external_declaration`, replace the literal-only validation for non-brace initializers (currently lines ~3642–3661) with a branch:

- If `decl->decl_type != TYPE_POINTER`: call `eval_const_expr(parser, "file-scope initializer")`, store the result as `AST_INT_LITERAL`
- Otherwise (pointer-typed): keep the existing `parse_assignment_expression` path with literal-only checks

**Task 3: Wire `eval_const_expr` to the multi-declarator file-scope path**

In `parse_external_declaration`, replace the literal-only validation in the comma-separated declarator list arm (currently lines ~3708–3716) with the same branch pattern, using the `k2` variable already computed for the second declarator.

---

## Required AST Changes

None. The result of `eval_const_expr` is stored as `AST_INT_LITERAL`, reusing the existing node type.

---

## Required Code Generation Changes

None. `AST_INT_LITERAL` at global scope is already handled by existing codegen.

---

## Test Plan

**Valid tests** (9):
1. Basic arithmetic: `int x = 10 * 3 + 2;`
2. Bitwise OR: `int MASK = 0xF0 | 0x0F;`
3. Left shift: `int PAGE = 1 << 12;`
4. sizeof(type): `int PTR_SZ = sizeof(void *);`
5. sizeof in expression: `int BUF = sizeof(int) * 256;`
6. sizeof struct: `struct Pair { int a; int b; }; int SZ = sizeof(struct Pair);`
7. Enum constant with operator: `enum { STEP = 5 }; int LIMIT = STEP * 2 + 1;`
8. Unary negation: `int NEG = -(3 * 4);`
9. Bitwise complement: `int ALL = ~0;`
10. Comma-separated declarators: `int A = 1 << 2, B = 3 * 7;`

**Invalid tests** (2):
1. File-scope variable reference (not a constant expression): `int n = 5; int m = n + 1;`
2. sizeof without parenthesized type: `int x = sizeof 5;`

**Print-AST tests** (1):
1. Constant folding verification: `int x = 1 + 2;` should show `IntLiteral: 3` in the AST, not a binary operation node

---

## Implementation Order

1. Add `TOKEN_SIZEOF` handling to `eval_const_primary` (Task 1)
2. Update the first-declarator file-scope initializer arm in `parse_external_declaration` (Task 2)
3. Update the multi-declarator file-scope initializer arm in `parse_external_declaration` (Task 3)
4. Bump `VERSION_STAGE` to `"01000000"` in `src/version.c`
5. Add tests
6. Update documentation (after all tests pass)

---

## Notes

- The `eval_const_expr` evaluator is already self-hosting-verified from stage 99
- `sizeof(type-name)` calls `parse_type_name`, which is also self-hosting-verified
- Bootstrap verification: run `./build.sh --mode=self-host` before declaring the stage complete
- No ambiguities or inconsistencies found in the spec
