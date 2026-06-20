# Stage 154 Kickoff: Unreachable Statement Removal

## Summary of the spec

Stage 154 implements dead-code elimination for statements that come after unconditional control-flow transfers in block scope. When the optimizer encounters a direct child of an `AST_BLOCK` that is terminal (`return`, `break`, `continue`, or `goto`), all subsequent siblings up to the next `AST_LABEL_STATEMENT` (exclusive) are detected as unreachable, freed via `ast_free`, and removed from the block's children array.

**Why this matters:** After stages 143–153 apply constant folding and dead-branch elimination, code that was conditionally reachable may now be provably dead textually. For example, `{ return 1; x = 99; }` still carries the dead assignment through codegen, producing redundant instructions. Stage 154 prunes these unreachable siblings at the AST level, reducing code size and improving readability of the emitted assembly.

**Scope:** Only `src/optimize.c` changes; no tokenizer, parser, AST, or codegen modifications. All removal is gated behind `-O1`; the `-O0` default path is unaffected.

---

## Required tokenizer changes

**None.** The tokenizer is unaffected.

---

## Required parser changes

**None.** The parser is unaffected. All AST node types (`AST_RETURN_STATEMENT`, `AST_BREAK_STATEMENT`, `AST_CONTINUE_STATEMENT`, `AST_GOTO_STATEMENT`, `AST_LABEL_STATEMENT`) already exist.

---

## Required AST changes

**None.** No new AST node types or fields are added. The optimizer works with existing node types and modifies the `child_count` and `children` array of `AST_BLOCK` nodes in-place.

---

## Required code generation changes

**None.** The existing codegen handlers remain unchanged. Codegen already respects `child_count` as the authoritative block size, so pruned children are never visited.

---

## Test plan

### Integration tests (all use `-O1`)

1. **test_unreachable_return**: Multiple dead statements after `return`
   - Function returns early; all statements after the return are unreachable
   - Expected output: `1`

2. **test_unreachable_break**: Dead statement after `break` in a loop
   - Loop exits on the first iteration; assignment after break does not execute
   - Expected output: `5`

3. **test_unreachable_continue**: Dead statement after `continue` in a loop
   - Loop continues; assignment after continue does not execute (increments occur first)
   - Expected output: `3`

4. **test_unreachable_goto**: Multiple dead statements between `goto` and its target label
   - Jump skips intervening assignments; only code after the label executes
   - Expected output: `1`

5. **test_unreachable_label_stop**: Multiple dead zones with labels as termination points
   - First `goto` creates a dead zone that stops at the first label; code after the label executes, then another `goto` creates a second dead zone
   - Expected output: `1 2`

### Regression testing

- Full test suite (`./run_tests.sh`) must pass at both `-O0` and `-O1`.
- Code at `-O0` is unaffected; the removal only occurs when `-O1` activates the optimizer.
- Labels must never be removed; a label stops the dead-code sweep, preserving itself and all subsequent code.
- Only direct children of `AST_BLOCK` are inspected; a nested block or control-flow statement that internally ends in a terminal does not trigger removal of outer siblings.
- Dead siblings with nested statements and declarations are fully freed (no memory leaks).

---

## Implementation notes

### Terminal statement types

A statement is terminal if its type is:
- `AST_RETURN_STATEMENT`
- `AST_BREAK_STATEMENT`
- `AST_CONTINUE_STATEMENT`
- `AST_GOTO_STATEMENT`

All other types (including `AST_LABEL_STATEMENT`) are non-terminal.

### Label stop rule

When a terminal is detected at position `i`, a forward scan begins at `i+1` to find the end of the dead zone:
- If an `AST_LABEL_STATEMENT` is encountered, the scan stops (label is reachable via `goto` elsewhere).
- If the block end is reached first, all remaining siblings are dead.

Dead siblings are freed in-place via `ast_free(node->children[k])`, then the survivors are compacted by shifting the label (if any) and all following siblings down to fill the gap.

### Multiple dead zones

If a block contains more than one dead zone (e.g., `return; a; label_x: return; b; label_y: c`), the outer iteration loop handles this naturally:
- First pass: terminal at position `i=0` → dead zone `[1 .. label_x-1]` → compact
- Second pass: terminal at new position of `label_x:return` → dead zone up to next label or end → compact
- Process continues until all iterations complete

### Memory management

Each dead child is freed immediately via `ast_free(node->children[k])`, which recursively frees all descendants. After the loop that frees dead siblings, a compaction loop shifts survivors: `node->children[j++] = node->children[k++]`. After compaction, `node->child_count = j` reflects the new size. The trailing unused slots in the children array are never accessed.

### Bootstrap compatibility

- Use `/* */` comments only (no `//`).
- Declare all loop variables (`int j, k;`) at the top of the `case AST_BLOCK: {}` block, before the main `for` loop.
- `ast_free` is declared in `"ast.h"` (already included).
- `is_terminal_stmt` is a new `static` helper in `src/optimize.c`; no header changes.
- `AST_RETURN_STATEMENT`, `AST_BREAK_STATEMENT`, `AST_CONTINUE_STATEMENT`, `AST_GOTO_STATEMENT`, `AST_LABEL_STATEMENT` are enum values in `"ast.h"` (already included).

---

## Implementation steps

1. Add the `is_terminal_stmt(ASTNode *node)` static helper to `src/optimize.c` before `optimize_stmt`. This function returns 1 if the node is one of the four terminal types, 0 otherwise.
2. In `optimize_stmt`, replace the `AST_BLOCK` case body: declare `int j, k;` at the top, then after each child is optimized, check if it is terminal; if so, scan forward, free dead siblings, and compact the children array.
3. Update the file-top comment block to document stage 154.
4. Build: `cmake --build build`.
5. Smoke test with a simple example: `{ return 1; x = 99; }` compiled with `-O1` should show no dead assignment.
6. Add the five integration test directories under `test/integration/`.
7. Run full test suite (`./run_tests.sh`).
8. Run self-hosting (`./build.sh --mode=self-host`).
9. Bump `VERSION_STAGE` to `"01540000"` in `src/version.c`.
10. Update `docs/outlines/checklist.md` and run the `update-supplemental-docs` skill.

---

## Out of scope

- **Block-ending-in-terminal at outer level**: Detecting that a nested block internally ends in a terminal does not make it terminal at the parent level; this requires recursive flow analysis.
- **Unreachable instructions in assembly**: Stripping dead `jmp`/`ret` instructions in the emitted assembly is a separate stage and explicitly out of scope.
- **Dead code warnings**: Emitting diagnostics for unreachable code is out of scope.
- **Dead code across labels**: Proving that a label is never the target of any `goto` requires interprocedural tracking and is out of scope.
