# Stage 147 Kickoff — Boolean / Logical Simplification

## Summary of the spec

Stage 147 extends the stage-142 optimizer with **boolean and logical simplification rules** for `&&`, `||`, and the unary `!` operator. All rules are gated behind `-O1`; the `-O0` path (default) remains unaffected. Only `src/optimize.c` changes.

Rule coverage for new (not previously handled) cases:
- **`!!x → (x != 0)`** — double logical NOT of a non-constant operand; produces a boolean normalization node.
- **`x && 0 → 0`** — right operand is zero constant; always-false result.
- **`x || nonzero → 1`** — right operand is any nonzero constant; always-true result.
- **`x && nonzero → (x != 0)`** — right operand is nonzero constant; reduce to boolean cast.
- **`x || 0 → (x != 0)`** — right operand is zero constant; reduce to boolean cast.

Previously-handled cases (do not re-implement):
- `!0 → 1`, `!nonzero_const → 0` — stage 144 constant unary folding.
- `!!const` — two applications of stage 144 unary folding.
- `0 && x → 0`, `nonzero || x → 1`, both-literal `&&`/`||` — stage 143 logical short-circuit folding.

---

## Required tokenizer changes

**None.**

---

## Required parser changes

**None.**

---

## Required AST changes

**None.** The stage creates new `AST_BINARY_OP("!=")` nodes for `!!x` using the existing `ast_new` + `ast_add_child` API. No new node types needed.

---

## Required code generation changes

**File: `src/optimize.c`**

**Task 1** — Insert a `!!x` block **after** the "Constant unary folding" block (stage 144) and **before** the "Algebraic identity folding" block (stage 145):
- Detects outer `AST_UNARY_OP("!")` whose child is also `AST_UNARY_OP("!")` with a non-literal child.
- Nulls the inner node's child slot before `ast_free` to prevent double-free of `x`.
- Builds a new `AST_BINARY_OP("!=")` node with `x` as left child and new `AST_INT_LITERAL("0")` as right child via `ast_add_child`.
- Declarations at top of outer `if` block for C0 compatibility: `inner`, `x`, `neq`, `zero`, `fire`.

**Task 2** — Insert a binary boolean block **after** the strength reduction block (stage 146) and **before** the final `return node;`:
- `x && 0 → 0`: free entire subtree, return new zero literal.
- `x || nonzero → 1`: free entire subtree, return new one literal.
- `x && nonzero → (x != 0)`: free right child, replace with new zero literal, mutate operator to `"!="`.
- `x || 0 → (x != 0)`: right child is already zero, just mutate operator to `"!="`.
- Declarations at top of outer `if` block: `op`, `right`, `right_is_lit`, `rval`, `right_is_zero`, `right_is_nonzero`, `z`, `zero_lit`.

---

## Test plan

Six new integration tests, all using `-O1`:

1. **test_bool_simplify_and_zero** — `x && 0 → 0`
2. **test_bool_simplify_or_nonzero** — `x || 1 → 1`, `x || 5 → 1`
3. **test_bool_simplify_and_one** — `x && 1 → (x != 0)` for zero and nonzero x
4. **test_bool_simplify_or_zero** — `x || 0 → (x != 0)` for zero and nonzero x
5. **test_bool_double_not** — `!!x` for nonzero, zero, and negative variables; `!!const` (already stage-144)
6. **test_bool_simplify_combined** — composed rules in a single expression

All existing tests must pass at both `-O0` and `-O1`.

---

## Implementation order

1. Write Task 1 (`!!x`) block in `src/optimize.c`
2. Write Task 2 (binary boolean) block in `src/optimize.c`
3. Build: `cmake --build build`
4. Smoke test with quick inline expressions
5. Add six integration tests
6. Run `bash test/run_all_tests.sh` — verify all pass
7. Run `./build.sh --mode=self-host` — verify C0→C1→C2
8. Bump `VERSION_STAGE` to `"01470000"` in `src/version.c`
9. Commit all changes
10. Generate milestone and transcript artifacts

---

## Ambiguities and decisions

**None found.** The spec is consistent and complete. Notable decisions made in the spec:

- Side effects of `x` in `x && 0` and `x || nonzero` are dropped — consistent with stage 145's aggressive folding policy (`0 * x → 0`).
- `!!x` fires only when the inner `!`'s child is not an `AST_INT_LITERAL`; the constant case is already fully handled by stage 144.
- The binary boolean block covers any nonzero constant (not just `1`), which is the natural generalization consistent with C semantics.
- Grammar is unchanged; no `docs/grammar.md` update needed.
