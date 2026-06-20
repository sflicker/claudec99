# Stage 152 Kickoff: Constant Propagation for `const` Scalar Locals

## Summary of the spec

Stage 152 implements constant propagation for `const`-qualified scalar local variables initialized with a compile-time integer literal. In `optimize_expr`, each `AST_VAR_REF` whose name is in the const table is replaced with a fresh `AST_INT_LITERAL`. This makes the constant visible to all downstream optimizer passes (binary folding, dead-branch elimination, etc.), enabling optimizations like `const int N = 8; int a = N * 4;` to fold to `32`.

**Why this matters:** Currently, `const` local variables are treated as opaque references; the optimizer cannot see their values. After this stage, substitution exposes the literal value to all downstream passes, enabling composition with stage-143 binary folding and stage-150 dead-branch elimination.

**Scope:** Only `src/optimize.c` changes; no tokenizer, parser, AST, or codegen modifications. All propagation is gated behind `-O1`; the `-O0` default path is unaffected.

---

## Required tokenizer changes

**None.** The tokenizer is unaffected.

---

## Required parser changes

**None.** The parser is unaffected. The `is_const` field on `ASTNode` is already set at parse time for const-qualified declarations.

---

## Required AST changes

**None.** No new AST node types or fields are added. The `is_const` field already exists and records const-qualification. The optimizer works with existing `AST_VAR_REF`, `AST_DECLARATION`, and `AST_BLOCK` nodes.

---

## Required code generation changes

**None.** The existing codegen handlers remain unchanged.

---

## Test plan

### Integration tests (all use `-O1`)

1. **test_const_prop_basic**: Direct use of const locals substituted verbatim
   - Variables: `const int x = 42`, `const long y = 100`
   - Expected: `42 100`

2. **test_const_prop_fold**: Const var in binary expression; substitution + stage-143 folding
   - Expressions: `const int N = 8; N * 4`, `N - 3`, `N + N`
   - Expected: `32 5 16`

3. **test_const_prop_dead_branch**: Const var in condition; substitution + stage-150 dead-branch elimination
   - Expressions: `const int FLAG = 0; if (FLAG)`, `const int LIMIT = 5; while (LIMIT - 5)`
   - Expected: `10 0`

4. **test_const_prop_scope**: Inner const shadows outer; scope restore after block
   - Nested blocks with `const int x = 10` shadowing outer `const int x = 5`
   - Expected two-line output: `10` then `5 5`

5. **test_const_prop_init_fold**: Const var initialized with foldable expression; after stage-143 folds initializer, records literal
   - Expression: `const int x = 3 * 4; x + 1` (initializer folds to 12, then 12 + 1 folds to 13)
   - Expected: `13`

### Regression testing

- Full test suite (`./run_tests.sh`) must pass at both `-O0` and `-O1`.
- Non-`const` variables must NOT be substituted.
- `const` aggregate variables (struct, array) must NOT be substituted.
- `const` pointer variables must NOT be substituted.

---

## Implementation notes

### Const propagation table

A file-static per-function table (`g_const_table`) maps variable names to recorded literal values. Each entry stores the variable name, numeric value, declared type, and unsigned flag.

**Scope handling:** `AST_BLOCK` saves the table length before processing children and restores it after, ensuring that inner-scope entries are invisible outside their block. Shadowing is handled by right-to-left scan (innermost entry wins).

### Recording eligibility

A declaration is eligible for propagation if:
1. It is `const`-qualified (`is_const == 1`)
2. Its type is a scalar integer type (`TYPE_BOOL`, `TYPE_CHAR`, `TYPE_SHORT`, `TYPE_INT`, `TYPE_LONG`, `TYPE_LONG_LONG`, `TYPE_UNSIGNED_LONG_LONG`)
3. It has an initializer that resolved to `AST_INT_LITERAL` (checked **after** `optimize_expr` is called on the initializer, so `const int x = 3 * 4;` is eligible: stage-143 folds `3 * 4` to `12` before the check runs)

### Key implementation details

- **Result type:** The substituted `AST_INT_LITERAL` carries `decl_type` and `is_unsigned` from the `AST_DECLARATION` node (the declared type of the variable), not from the initializer.
- **Recording after optimization:** The declaration's initializer is passed through `optimize_expr` first; if it folds to `AST_INT_LITERAL`, it is recorded. This incidentally handles `const int x = 3*4;` (folded to 12 by stage 143 before recording).
- **Memory management:** The `g_const_table` stores `node->value` directly without copying (safe because the `AST_DECLARATION` node remains in the AST). When a `AST_VAR_REF` is substituted, a fresh `AST_INT_LITERAL` is allocated and the old node is freed.
- Use the same memory-management idiom as stages 145, 149, and 151: call `ast_free(node)` then return a freshly allocated `AST_INT_LITERAL`.

### Bootstrap compatibility

- Use `/* */` comments only (no `//`).
- Declare all variables at the top of each block before executable statements.
- The `ConstEntry` typedef and table variables must appear at file scope before any function definition.
- All required AST and type constants are already declared in `"ast.h"`.
- `strcmp`, `strtol`, `snprintf`, `util_strdup` are already available.

---

## Implementation steps

1. Add static table, macro, typedef, and helpers to `src/optimize.c` before `optimize_expr`:
   - `CONST_PROP_MAX` macro (64)
   - `ConstEntry` typedef with `name`, `value`, `decl_type`, `is_unsigned` fields
   - `g_const_table[]` and `g_const_count` file-static variables
   - `is_scalar_int_type(TypeKind)` helper
   - `const_prop_lookup(const char *name)` helper (scans right-to-left)
2. In `optimize_expr`: add `AST_VAR_REF` substitution block immediately before the final `return node;`.
3. In `optimize_stmt`: split the combined `AST_DECLARATION` / `AST_DECL_LIST` case; add recording logic to `AST_DECLARATION`.
4. In `optimize_stmt`: add scope save/restore to `AST_BLOCK` case.
5. In `optimize_translation_unit`: add `g_const_count = 0;` before each function-body optimization.
6. Update the file-top comment to include: `Stage 152: const propagation -- const scalar locals with literal init replaced at each AST_VAR_REF.`
7. Smoke test with a simple const propagation example using `-O1`.
8. Add the five integration test directories under `test/integration/`.
9. Run full test suite (`./run_tests.sh`).
10. Run self-hosting (`./build.sh --mode=self-host`).
11. Bump `VERSION_STAGE` to `"01520000"` in `src/version.c`.
12. Update `docs/outlines/checklist.md` and run the `update-supplemental-docs` skill.
