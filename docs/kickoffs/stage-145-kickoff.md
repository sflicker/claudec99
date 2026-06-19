# Stage 145 Kickoff — Algebraic Identity Folding

## Summary of the spec

Stage 145 extends the stage-142 optimizer with **algebraic identity folding**: simplifying binary operations when one or both operands are constants or structurally identical variable references. All folding is gated behind `-O1`; the `-O0` path (default) remains unaffected. Only `src/optimize.c` changes.

Rule coverage includes:
- **Additive identities**: `x + 0 → x`, `0 + x → x`, `x - 0 → x`
- **Multiplicative identities**: `x * 1 → x`, `1 * x → x`, `x / 1 → x`
- **Zero propagation**: `x * 0 → 0`, `0 * x → 0`, `0 / x → 0`
- **Self-cancellation**: `x - x → 0`, `x ^ x → 0`
- **Bitwise zero**: `x & 0 → 0`, `0 & x → 0`, `x | 0 → x`, `0 | x → x`
- **Identity masks**: `x & -1 → x` (where `-1` is the folded result of `~0`)

---

## Required tokenizer changes

**None.** No new tokens or token modifications needed.

---

## Required parser changes

**None.** No parser changes required.

---

## Required AST changes

**None.** No AST node type or structure changes required.

---

## Required code generation changes

**File: `src/optimize.c`**

Insert an algebraic identity folding block in `optimize_expr` after the "Constant unary folding" block and before the final `return node;`.

The block:
- Detects when a binary operation has a constant literal or matching variable operands
- Applies identity rules (returning an existing child with proper memory management)
- Applies zero rules (creating fresh zero literals with inherited type information)
- Uses `strcmp` on operator strings and `strtol` for literal value parsing

Key implementation details:
- Before `ast_free(node)`, null out the kept child's slot to prevent double-free
- New zero literals inherit `decl_type` and `is_unsigned` from the appropriate operand
- Self-cancellation checks only structural equality (both children are `AST_VAR_REF` with same `value`)
- All code must remain C89/C99 compatible with no VLAs or single-line comments

---

## Test plan

Six new integration tests, all using `-O1`:

1. **test_fold_additive_identity** — tests `x + 0`, `0 + x`, `x - 0`
2. **test_fold_multiplicative_identity** — tests `x * 1`, `1 * x`, `x / 1`
3. **test_fold_zero_propagation** — tests `x * 0`, `0 * x`, `0 / x`
4. **test_fold_self_cancellation** — tests `x - x`, `x ^ x`, `x & 0`, `x | 0`
5. **test_fold_identity_mask** — tests `x & ~0` (folded to `x & -1`)
6. **test_fold_algebraic_combined** — verifies rule composition in a single bottom-up pass

All existing tests must pass at both `-O0` and `-O1`.

---

## Implementation order

1. Edit `src/optimize.c`: insert algebraic identity block
2. Build: `cmake --build build`
3. Smoke test with a simple `-O1` expression
4. Add integration tests under `test/integration/`
5. Run `./run_tests.sh` — verify all pass
6. Run `./build.sh --mode=self-host` — verify C0→C1→C2
7. Bump `VERSION_STAGE` in `src/version.c`
8. Update docs: checklist, self-compilation report

---

## Ambiguities and decisions

**None found.** The spec clearly defines all 16 rules, memory management protocol, type inheritance convention, and bootstrap compatibility requirements. The implementation block is provided verbatim in the spec.

Key decisions documented in spec:
- Self-cancellation intentionally excludes side-effecting expressions (shallow structural check only)
- The `0 / x → 0` fold is valid UB assumption per C standard
- No overflow detection or warnings in this stage
- Type inheritance for new zero literals uses left operand for symmetric ops, right operand for asymmetric cases
