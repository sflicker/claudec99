# Stage 149 Kickoff — Conditional Expression Folding

## Summary of the spec

Stage 149 extends the stage-142 optimizer with **conditional expression folding**: reducing `const ? T : F` to the selected branch `T` or `F` when the condition is a compile-time integer constant. All folding is gated behind `-O1`; the `-O0` path (default) remains unaffected. Only `src/optimize.c` changes.

Rule coverage for new (not previously handled) cases:
- **`nonzero ? T : F → T`** — when condition is `AST_INT_LITERAL` with value ≠ 0; the false branch `F` is freed.
- **`0 ? T : F → F`** — when condition is `AST_INT_LITERAL` with value == 0; the true branch `T` is freed.

Previously-handled cases (do not re-implement):
- Constant conditions arising from stage-143 (binary folding), stage-144 (unary folding), or stage-147 (logical simplification).

---

## Required tokenizer changes

**None.**

---

## Required parser changes

**None.**

---

## Required AST changes

**None.** The stage returns the existing branch node directly; no new node types needed.

---

## Required code generation changes

**File: `src/optimize.c`**

**Task** — Insert a constant-condition ternary block **after** the "Boolean/logical simplification" block (stage 147) and **before** the `return node;` that ends `optimize_expr`:
- Detects `AST_CONDITIONAL_EXPR` whose condition child has been folded to `AST_INT_LITERAL`.
- Selects the true branch (index 1) for nonzero, the false branch (index 2) for zero.
- Nulls the selected branch's slot before `ast_free` to prevent double-free.
- Returns the selected branch node directly.
- Declarations at top of outer `if` block for C0 compatibility: `cval`, `keep_idx`, `keep`.

---

## Test plan

Four new integration tests, all using `-O1`:

1. **test_cond_fold_true** — `1 ? 42 : 99`, `7 ? 10 : 20`, `-1 ? 5 : 6` all select the true branch; expected output `42 10 5`.
2. **test_cond_fold_false** — `0 ? 42 : 99`, `0 ? 10 : 20` both select the false branch; expected output `99 20`.
3. **test_cond_fold_nested** — nested ternaries with constant conditions fold bottom-up: `1 ? 1 : (0 ? 2 : 3)` and `0 ? (1 ? 2 : 3) : 4` reduce correctly to `1` and `4`.
4. **test_cond_fold_combined** — ternary condition derived from prior folding: `(2 + 3) ? 10 : 20` and `(1 - 1) ? 10 : 20` composed with stage-143 binary folding to produce correct results `10` and `20`.

All existing tests must pass at both `-O0` and `-O1`.

---

## Implementation order

1. Edit `src/optimize.c`: insert constant-condition ternary block after the stage-147 block
2. Build: `cmake --build build`
3. Smoke test with a simple `-O1` ternary expression
4. Add four integration tests under `test/integration/`
5. Run `./run_tests.sh` — verify all pass
6. Run `./build.sh --mode=self-host` — verify C0→C1→C2
7. Bump `VERSION_STAGE` to `"01490000"` in `src/version.c`
8. Update docs: checklist, self-compilation report

---

## Ambiguities and decisions

**None found.** The spec is clear and complete. Notable decisions:

- The condition must be `AST_INT_LITERAL` after the bottom-up walk; conditions that are non-constant or non-folded remain as-is.
- The surviving branch node is returned directly (not a new node); the branch already carries the correct `decl_type` and `is_unsigned` from parsing.
- Memory management pattern is identical to stage 148's `-(-x)` block, with the dead branch freed before the `?:` node itself.
- Grammar is unchanged; no `docs/grammar.md` update needed.
