# Stage 153 Kickoff: Cast Constant Folding

## Summary of the spec

Stage 153 implements constant folding for `AST_CAST` nodes whose operand has been reduced to `AST_INT_LITERAL`. When the cast target is a scalar integer type and the operand value is preserved by the cast (i.e., fits in the target type without truncation or sign change), the `AST_CAST` is replaced with a fresh `AST_INT_LITERAL` carrying the target type.

**Why this matters:** Currently, casts of compile-time constants are left unfoldable; the optimizer cannot see them. For example, `(int)sizeof(long) == 8` remains as a cast expression that stage-150 cannot reduce to `1`. After this stage, casts of literals fold immediately, enabling composition with stage-151 (sizeof folding), stage-152 (const propagation), stage-143 (binary folding), and stage-150 (dead-branch elimination) in a single pass.

**Scope:** Only `src/optimize.c` changes; no tokenizer, parser, AST, or codegen modifications. All folding is gated behind `-O1`; the `-O0` default path is unaffected.

---

## Required tokenizer changes

**None.** The tokenizer is unaffected.

---

## Required parser changes

**None.** The parser is unaffected. `AST_CAST` nodes and their `decl_type`, `is_unsigned` fields are already set at parse time.

---

## Required AST changes

**None.** No new AST node types or fields are added. The optimizer works with existing `AST_CAST` and `AST_INT_LITERAL` nodes.

---

## Required code generation changes

**None.** The existing codegen handlers remain unchanged. Unsafe casts (where the value would truncate or sign-change) are left unfoldable and codegen handles them as before.

---

## Test plan

### Integration tests (all use `-O1`)

1. **test_cast_fold_basic**: Direct cast of an integer literal; produces a literal of the target type
   - Casts: `(int)42L`, `(long)7`, `(int)100LL`
   - Expected: `42 7 100`

2. **test_cast_fold_sizeof**: Cast of a `sizeof` result; requires stage-151 to fold sizeof, stage-153 to fold the cast, then stage-143 to fold the binary expression
   - Expressions: `(int)sizeof(long) + 1`, `(int)sizeof(int) * 2`, `(long)sizeof(char) + 7L`
   - Expected: `9 8 8`

3. **test_cast_fold_const_prop**: Cast of a const-propagated variable; requires stage-152 to substitute `VAR_REF` first, then stage-153 to fold through the cast
   - Expressions: `const long N = 100; (int)N`, `(int)N + 5`
   - Expected: `100 105`

4. **test_cast_fold_dead_branch**: Cast in a condition; stage-151 folds sizeof, stage-153 folds the cast, stage-143 folds the comparison, stage-150 eliminates the dead branch
   - Expressions: `(int)sizeof(long) == 8` (keeps then-branch), `(int)sizeof(int) == 8` (keeps else-branch)
   - Expected: `1 2`

5. **test_cast_fold_unsafe**: Casts that change the value must NOT be folded; codegen handles truncation and unsigned wrap
   - Casts: `(char)300` (truncates to implementation-defined result 44), `(unsigned char)(-1)` (wraps to 255)
   - Expected: `44 255`

### Regression testing

- Full test suite (`./run_tests.sh`) must pass at both `-O0` and `-O1`.
- Casts to `float`, `double`, and pointer types must NOT be folded.
- Casts of non-literal operands (variable references, binary expressions, function calls) must NOT be folded.
- Unsafe casts (truncating or sign-changing) must NOT be folded and must produce correct output via codegen.

---

## Implementation notes

### Eligibility criteria

An `AST_CAST` node is folded if and only if all of the following hold (after bottom-up recursion):

1. Node type is a cast (`node->type == AST_CAST`)
2. Operand is a literal (`node->children[0]->type == AST_INT_LITERAL`)
3. Target is scalar integer (`is_scalar_int_type(node->decl_type)` returns 1; reuses the stage-152 helper)
4. Value fits in target type (`cast_value_safe(val, node->decl_type, node->is_unsigned)` returns 1)

### Value-safe ranges

The helper `cast_value_safe` checks that the integer value of the operand literal is numerically identical before and after the cast:

- `TYPE_BOOL`: `val == 0 || val == 1`
- `TYPE_CHAR` (signed): `-128 ≤ val ≤ 127`
- `TYPE_CHAR` (unsigned): `0 ≤ val ≤ 255`
- `TYPE_SHORT` (signed): `-32768 ≤ val ≤ 32767`
- `TYPE_SHORT` (unsigned): `0 ≤ val ≤ 65535`
- `TYPE_INT` (signed): `-2147483648 ≤ val ≤ 2147483647`
- `TYPE_INT` (unsigned): `0 ≤ val ≤ 4294967295`
- `TYPE_LONG` (signed): always (signed 64-bit; `long` stores value exactly)
- `TYPE_LONG` (unsigned): `val ≥ 0`
- `TYPE_LONG_LONG` (signed): always
- `TYPE_LONG_LONG` (unsigned): `val ≥ 0`
- `TYPE_UNSIGNED_LONG_LONG`: `val ≥ 0`

### Key implementation details

- **Result type:** The new `AST_INT_LITERAL` carries `decl_type` and `is_unsigned` from the `AST_CAST` node (the target type), not from the source operand.
- **Value string:** The formatted value string is `snprintf`-ed from the `long val` read from the source literal. Since the safe-range check guarantees `val` is unchanged, the printed decimal is the same number, just with a potentially different type annotation.
- **Memory management:** The `val` is read from `node->children[0]->value` via `strtol` before freeing. The `dst_type` and `dst_unsigned` are read from `node` before freeing. `ast_free(node)` frees the `AST_CAST` node, its children array, and the child `AST_INT_LITERAL`, but not the `value` strings (they are either string literals or allocations the optimizer never frees). The new literal is allocated with `ast_new(AST_INT_LITERAL, util_strdup(buf))`.

### Bootstrap compatibility

- Use `/* */` comments only (no `//`).
- Declare all variables at the top of each block before executable statements.
- `strtol` and `snprintf` are in `<stdlib.h>` and `<stdio.h>` (already included).
- `util_strdup` is declared in `"util.h"` (already included).
- `ast_new`, `ast_free`, `AST_CAST`, `AST_INT_LITERAL`, `TYPE_BOOL`, `TYPE_CHAR`, `TYPE_SHORT`, `TYPE_INT`, `TYPE_LONG`, `TYPE_LONG_LONG`, `TYPE_UNSIGNED_LONG_LONG` are in `"ast.h"` (already included).
- `is_scalar_int_type` is the stage-152 static helper already in this file; reuse it without redeclaration.

---

## Implementation steps

1. Add the `cast_value_safe` static helper to `src/optimize.c` before `optimize_expr` (after the existing `const_prop_lookup` helper).
2. In `optimize_expr`, insert the `AST_CAST` folding block immediately after the `AST_SIZEOF_EXPR` block and before the `AST_VAR_REF` const-propagation block.
3. Update the file-top comment block to document stage 153.
4. Build: `cmake --build build`.
5. Smoke test with a simple cast-folding example using `-O1`.
6. Add the five integration test directories under `test/integration/`.
7. Run full test suite (`./run_tests.sh`).
8. Run self-hosting (`./build.sh --mode=self-host`).
9. Bump `VERSION_STAGE` to `"01530000"` in `src/version.c`.
10. Update `docs/outlines/checklist.md` and run the `update-supplemental-docs` skill.
