# Stage 148 Kickoff — Negation Folding

## Summary of the spec

Stage 148 extends the stage-142 optimizer with **negation folding**: reducing `-(-x)` to `x` for non-constant expressions `x`. This is the arithmetic analogue of the `!!x` (double logical NOT) rule added in Stage 147. All folding is gated behind `-O1`; the `-O0` path (default) remains unaffected. Only `src/optimize.c` changes.

Rule coverage for new (not previously handled) cases:
- **`-(-x) → x`** — double arithmetic negation of a non-constant operand; returns the inner operand directly.

Previously-handled cases (do not re-implement):
- `-const` and `-(-const)` — stage 144 constant unary folding (two-pass).
- `!!x` — stage 147 double logical NOT folding.

---

## Required tokenizer changes

**None.**

---

## Required parser changes

**None.**

---

## Required AST changes

**None.** The stage returns the existing inner node directly; no new node types needed.

---

## Required code generation changes

**File: `src/optimize.c`**

**Task 1** — Insert a `-(-x)` block **after** the "Double logical NOT" block (stage 147) and **before** the "Algebraic identity folding" block (stage 145):
- Detects outer `AST_UNARY_OP("-")` whose child is also `AST_UNARY_OP("-")` with a non-literal child.
- Nulls the inner node's child slot before `ast_free` to prevent double-free of `x`.
- Returns the inner operand `x` directly.
- Declarations at top of outer `if` block for C0 compatibility: `inner`, `x`, `fire`.

---

## Test plan

Three new integration tests, all using `-O1`:

1. **test_neg_fold_double_minus** — `x = -(-5)` and `y = -(-var)` produce correct values; `-(-0)` and `-(-7)` handled by stage 144.
2. **test_neg_fold_triple_minus** — `x = -(-(-x))` reduces to `-x` (inner pair folds first, leaving single `-`).
3. **test_neg_fold_combined** — double-negation folding composes with algebraic identity rules: `-(-x) + 0 → x`, `-(-x) * 1 → x`, `-(-x) - x → 0`.

All existing tests must pass at both `-O0` and `-O1`.

---

## Implementation order

1. Edit `src/optimize.c`: insert `-(-x)` block after the stage-147 double logical NOT block
2. Build: `cmake --build build`
3. Smoke test with a simple `-O1` double-negation expression
4. Add three integration tests under `test/integration/`
5. Run `./run_tests.sh` — verify all pass
6. Run `./build.sh --mode=self-host` — verify C0→C1→C2
7. Bump `VERSION_STAGE` to `"01480000"` in `src/version.c`
8. Update docs: checklist, self-compilation report

---

## Ambiguities and decisions

**None found.** The spec is clear and complete. Notable decisions:

- When `x` is a compile-time integer literal, stage 144's two-pass constant unary folding already reduces `-(-const)` to the literal; this rule fires only for non-literal `x`.
- The rule returns `x` itself (not a new node); no type adjustment needed.
- Memory management pattern is identical to stage 147's `!!x` block, with `"-"` substituted for `"!"` and direct return instead of building a `!=` node.
- Grammar is unchanged; no `docs/grammar.md` update needed.
