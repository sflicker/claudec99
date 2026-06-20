# Stage 151 Kickoff: sizeof Constant Folding

## Summary of the spec

Stage 151 implements `sizeof` constant folding in the optimizer (`src/optimize.c`). The goal is to replace `AST_SIZEOF_TYPE` and `AST_SIZEOF_EXPR` nodes with `AST_INT_LITERAL` nodes wherever the size can be determined at compile time without runtime symbol-table access.

**Why this matters:** Currently, the codegen emits `mov rax, N` directly for sizeof expressions, making the constant value invisible to optimizer passes. After this stage, expressions like `sizeof(long) == 8` or `sizeof(int) * 64` become AST_INT_LITERAL nodes, enabling composition with stage-143 binary folding and stage-150 dead-branch elimination.

**Scope:** Only `src/optimize.c` changes; no tokenizer, parser, AST, or codegen modifications. All folding is gated behind `-O1`; the `-O0` default path is unaffected.

---

## Required tokenizer changes

**None.** The tokenizer is unaffected.

---

## Required parser changes

**None.** The parser is unaffected. Type information (`decl_type`, `full_type`) is already set by the parser and stored on sizeof nodes.

---

## Required AST changes

**None.** No new AST node types or fields are added. The optimizer works with existing `AST_SIZEOF_TYPE` and `AST_SIZEOF_EXPR` nodes and replaces them with `AST_INT_LITERAL` nodes.

---

## Required code generation changes

**None.** The existing codegen handlers for `AST_SIZEOF_TYPE` and `AST_SIZEOF_EXPR` remain unchanged as fallback paths for unfolded cases (e.g., `sizeof(variable)` where the optimizer has no symbol-table access).

---

## Test plan

### Integration tests (all use `-O1`)

1. **test_sizeof_type_fold**: `sizeof(type)` folded and composed with stage-143 binary folding
   - Expressions: `sizeof(int) * 2`, `sizeof(long) - sizeof(char)`, `sizeof(void *) / 2`
   - Expected: `8 7 4`

2. **test_sizeof_expr_fold**: `sizeof` in arithmetic with literal operands
   - Expressions: `sizeof(int) * 8`, `sizeof(long) + sizeof(int)`
   - Expected: `32 12`

3. **test_sizeof_dead_branch**: `sizeof` in conditional expressions (stage-150 composition)
   - Branch elimination: `if (sizeof(long) == 8)`, `if (sizeof(int) == 8)`
   - Expected: `10 20`

4. **test_sizeof_string_fold**: `sizeof(string-literal)` folding
   - Expressions: `sizeof("hi")`, `sizeof("")`
   - Expected: `3 1`

5. **test_sizeof_struct_fold**: `sizeof(struct-type)` and `sizeof(array-type)` folding
   - Expressions: `sizeof(struct Point)`, `sizeof(int[4])`
   - Expected: `8 16`

### Regression testing

- Full test suite (`./run_tests.sh`) must pass at both `-O0` and `-O1`.
- Variable-reference sizeof (e.g., `sizeof(x)` where `x` is a local variable) must still produce correct sizes via the codegen fallback path.

---

## Implementation notes

### Folding rules

**`AST_SIZEOF_TYPE` (always foldable):** All type information is present on the node. Use a new `sizeof_scalar_size(TypeKind)` helper to map type kinds to byte sizes. For struct/union/array types, use `node->full_type->size`.

**`AST_SIZEOF_EXPR` (partially foldable):** Only foldable for literal operands:
- `AST_STRING_LITERAL`: `byte_length + 1`
- `AST_INT_LITERAL`: scalar size from `child->decl_type`
- `AST_CHAR_LITERAL`: always 4 (char promotes to int in sizeof)
- Other operands: fall through to existing codegen handler

### Key implementation details

- Replacement literal must have `decl_type = TYPE_LONG` and `is_unsigned = 1` to match sizeof semantics (yields `unsigned size_t`).
- `sizeof_scalar_size` includes an explicit `TYPE_DOUBLE: return 8` case, correcting a latent codegen bug (the codegen switch falls through to `default: sz = 4` for double).
- Use the same memory-management idiom as stages 145 and 149: call `ast_free(node)` then return a freshly allocated `AST_INT_LITERAL`.
- `ast_free` does not free `node->full_type` (owned by the parser's type tables); no changes to `ast_free` needed.

### Bootstrap compatibility

- Use `/* */` comments only (no `//`).
- Declare all variables at the top of each block before executable statements.
- All required AST and type constants are already declared in `"ast.h"`.
- `snprintf` is in `<stdio.h>`, already included.

---

## Implementation steps

1. Add static helpers `sizeof_scalar_size` and `make_sizeof_literal` to `src/optimize.c` before `optimize_expr`.
2. Insert `AST_SIZEOF_TYPE` folding block immediately before the final `return node;` in `optimize_expr`.
3. Insert `AST_SIZEOF_EXPR` folding block immediately after the `AST_SIZEOF_TYPE` block.
4. Update the file-top comment to include: `Stage 151: sizeof constant folding -- AST_SIZEOF_TYPE/EXPR -> AST_INT_LITERAL.`
5. Smoke test with a simple sizeof expression using `-O1`.
6. Add the five integration test directories under `test/integration/`.
7. Run full test suite (`./run_tests.sh`).
8. Run self-hosting (`./build.sh --mode=self-host`).
9. Bump `VERSION_STAGE` to `"01510000"` in `src/version.c`.
10. Update `docs/outlines/checklist.md` and run the `update-supplemental-docs` skill.
