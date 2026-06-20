# Stage 150 Kickoff — Dead-Branch Elimination

## Summary of the spec

Stage 150 extends the stage-142 optimizer with **dead-branch elimination**: reducing control-flow statements (`if`, `while`, `for`) whose condition has been folded to a compile-time integer constant. When the condition is `AST_INT_LITERAL` after optimization:

- **`if (nonzero) { S1 } [else { S2 }]`** → keep `S1`, free `S2` (or empty block if no else)
- **`if (0) { S1 } [else { S2 }]`** → keep `S2` (or empty block), free `S1`
- **`while (0) { S }`** → empty block (body freed)
- **`for (init; 0; update) { S }`** → `init` (or empty block), body and update freed

Non-zero constant `while` and `for` conditions are not eliminated (infinite loops are observable). Only the `-O1` path is affected; `-O0` remains unchanged. Only `src/optimize.c` changes.

---

## Required tokenizer changes

**None.**

---

## Required parser changes

**None.**

---

## Required AST changes

**None.** The stage returns existing branch/init nodes directly; empty blocks are created via `ast_new(AST_BLOCK, NULL)`.

---

## Required optimizer/code generation changes

**File: `src/optimize.c`**

Three case replacements in `optimize_stmt`:

### 1. `AST_IF_STATEMENT` case

After child optimization, detect `AST_INT_LITERAL` condition:
- If nonzero: null out then-branch slot, `ast_free(node)`, return then-branch.
- If zero: null out else-branch slot (if present), `ast_free(node)`, return else-branch or empty block.

### 2. `AST_WHILE_STATEMENT` case

After child optimization, detect `AST_INT_LITERAL` condition with value 0:
- `ast_free(node)`, return empty block.

### 3. `AST_FOR_STATEMENT` case

After child optimization, detect `AST_INT_LITERAL` condition with value 0:
- Detach init (null out slot), `ast_free(node)`, return init or empty block.

Memory management pattern (identical to stage 149):
1. Save pointer to node(s) to keep.
2. Null their slots before `ast_free` to prevent double-free.
3. Call `ast_free(node)` to free parent and dead children.
4. Return the kept node(s).

Bootstrap constraints:
- `/* */` comments only, no `//`
- Declarations at block top before statements (new scopes with `case` blocks)
- `strtol` already included via `<stdlib.h>`; `ast_free`, `ast_new` already declared

---

## Test plan

Six new integration tests, all using `-O1`:

1. **test_dead_if_true** — `if (1) { a = 10; } else { a = 99; }` and `if (-3) { b = 20; } else { b = 99; }` select true branch; expected output `10 20`.

2. **test_dead_if_false** — `if (0) { a = 99; } else { a = 10; }` and `if (0) { b = 99; }` select false branch or empty; expected output `10 0`.

3. **test_dead_while** — `while (0) { n = 99; }` never executes; expected output `0`.

4. **test_dead_for** — `for (x = 5; 0; x++) { n = 99; }` runs init, skips body; expected output `5 0`.

5. **test_dead_for_no_init** — `for (; 0; ) { n = 99; }` entire loop eliminated; expected output `0`.

6. **test_dead_combined** — composed with stage-143 constant folding: `if (3 - 3) { ... } else { ... }` and `while (5 - 5) { ... }` produce folded conditions that feed dead-branch elimination; expected output `10 0`.

All existing tests must pass at both `-O0` and `-O1`.

---

## Implementation order

1. Edit `src/optimize.c`: replace `AST_IF_STATEMENT` case
2. Edit `src/optimize.c`: replace `AST_WHILE_STATEMENT` case
3. Edit `src/optimize.c`: replace `AST_FOR_STATEMENT` case
4. Build: `cmake --build build`
5. Smoke test: quick `-O1` compile and run with a test program
6. Add six integration tests under `test/integration/`
7. Run `./run_tests.sh` — verify all pass
8. Run `./build.sh --mode=self-host` — verify C0→C1→C2
9. Bump `VERSION_STAGE` to `"01500000"` in `src/version.c`
10. Update `docs/outlines/checklist.md` and run `update-supplemental-docs` skill
11. Update `docs/self-compilation-report.md` with stage-150 self-host results
12. Commit feature changes and self-host report

---

## Ambiguities and decisions

**None found.** The spec is clear and complete. Notable design decisions:

- Only zero constant while/for conditions are eliminated (non-zero constant loops are observable).
- Dead branches are freed via `ast_free` after detaching live child pointers.
- Empty placeholder blocks use `ast_new(AST_BLOCK, NULL)` for uniform handling.
- No grammar changes; control-flow syntax is unchanged.
- Side effects in dead branches are safely dropped (unreachable at runtime due to compile-time constant condition).
