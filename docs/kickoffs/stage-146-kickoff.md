# Stage 146 Kickoff — Strength Reduction: Power-of-Two Multiply/Divide

## Summary of the spec

Stage 146 extends the stage-142 optimizer with **strength reduction**: replacing multiplication by a compile-time power-of-two constant with a left shift, and replacing division by a compile-time power-of-two constant (for unsigned or statically non-negative dividends) with a right shift. All folding is gated behind `-O1`; the `-O0` path (default) remains unaffected. Only `src/optimize.c` changes.

Rule coverage includes:
- **Multiply by power of two**: `x * 2^N → x << N` (N ≥ 1)
- **Multiply by power of two (commutative)**: `2^N * x → x << N` (N ≥ 1)
- **Divide by power of two**: `x / 2^N → x >> N` (unsigned dividend or statically non-negative literal dividend, N ≥ 1)

The `* 1` and `/ 1` cases (`2^0`) are already handled by stage-145 algebraic identity rules and do not reach this block.

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

Insert a strength reduction block in `optimize_expr` after the "Algebraic identity folding" block and before the final `return node;`.

The block:
- Detects when a `*` or `/` operation has an `AST_INT_LITERAL` right operand (or left operand for commutative multiply) that is an exact power of two
- Identifies power-of-two literals using `(v > 1) && ((v & (v - 1)) == 0)` check
- Computes shift amount via bit-counting loop
- For multiply: replaces operator with `<<` and right operand with shift amount
- For divide: replaces operator with `>>` and right operand with shift amount (only when dividend is unsigned or statically non-negative literal)
- Correctly manages memory when freeing old operands and creating new shift-amount literals

Key implementation details:
- Compute power-of-two detection and shift amount at block entry (all declarations at top for C0 compatibility)
- For `x * 2^N` and `x / 2^N`: free right operand, replace with new literal
- For `2^N * x`: reorder children (x to left slot), free left operand (power-of-two literal)
- New shift-amount literals are `TYPE_INT` / not unsigned (consistent with parser convention)
- Preserve original node's `decl_type` and `is_unsigned` for result type
- All code must remain C89 compatible with no VLAs or single-line comments

---

## Test plan

Five new integration tests, all using `-O1`:

1. **test_strength_reduce_mul_pow2** — tests `x * 2`, `x * 4`, `x * 8`, `x * 16`
2. **test_strength_reduce_mul_pow2_commutative** — tests `2 * x`, `8 * x`
3. **test_strength_reduce_div_pow2_unsigned** — tests `x / 2`, `x / 4`, `x / 8` with unsigned operands
4. **test_strength_reduce_no_signed_div** — regression test verifying signed division by power of two is NOT reduced (dividend is signed variable with unknown sign at compile time)
5. **test_strength_reduce_combined** — verifies strength reduction composes with constant folding in a single bottom-up pass (e.g., `x * (1 << 3)` where inner shift folds to 8, then outer multiply reduces to shift)

All existing tests must pass at both `-O0` and `-O1`.

---

## Implementation order

1. Edit `src/optimize.c`: insert strength reduction block after algebraic identity block
2. Build: `cmake --build build`
3. Smoke test with a simple `-O1` multiply and divide expression
4. Add integration tests under `test/integration/`
5. Run `./run_tests.sh` — verify all pass
6. Run `./build.sh --mode=self-host` — verify C0→C1→C2
7. Bump `VERSION_STAGE` to `"01460000"` in `src/version.c`
8. Update docs: checklist, self-compilation report

---

## Ambiguities and decisions

**None found.** The spec is unambiguous and includes:
- Exact power-of-two detection formula
- Shift-amount computation via bit-counting loop
- Memory management protocol for all three rule forms
- Bootstrap compatibility constraints (C0-safe declarations, no VLAs, no single-line comments)
- Clear scope boundary for signed division (only static non-negativity recognized)

Key decisions documented in spec:
- Signed division by power-of-two only reduced when dividend is `unsigned` or an `AST_INT_LITERAL` with non-negative value; runtime-signed variables are out of scope
- The new rule block fires bottom-up after algebraic identity folding, so constant-folded powers-of-two (e.g., from `1 << 3`) are eligible for strength reduction in the same pass
- `* 1` and `/ 1` never reach this block; handled by stage-145 identity rules
