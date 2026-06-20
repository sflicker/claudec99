# stage-153 Transcript: Cast Constant Folding

## Summary

Stage 153 extends the stage-142 AST-level optimizer with cast constant folding. When an `AST_CAST` node to a scalar integer type has an `AST_INT_LITERAL` child whose numeric value fits exactly in the target type, `optimize_expr` replaces the entire cast node with a fresh `AST_INT_LITERAL` carrying the cast's target `decl_type` and `is_unsigned`. Unsafe casts — those where the value would be truncated, sign-changed, or unsigned-wrapped (e.g., `(char)300`, `(unsigned char)(-1)`) — are left unchanged and handled correctly by codegen as before.

Because the optimizer walks the AST bottom-up, child expressions are fully folded before the parent `AST_CAST` is inspected. This means `sizeof→literal→cast→literal` and `const-prop→literal→cast→literal` chains all resolve in a single pass, immediately exposing the result to downstream folding rules: stage-143 binary folding, stage-150 dead-branch elimination, etc.

The change is confined entirely to `src/optimize.c`. No tokenizer, parser, AST, or codegen changes were needed. The self-host C0→C1→C2 cycle passed cleanly with no source changes required during bootstrap.

## Changes Made

### Step 1: `src/optimize.c` — `cast_value_safe` helper

- Added `static int cast_value_safe(long val, TypeKind target, int target_unsigned)` after `const_prop_lookup` and before `sizeof_scalar_size`
- Returns 1 when `val` can be represented exactly in the target integer type:
  - `TYPE_BOOL`: only 0 or 1
  - `TYPE_CHAR`: signed -128..127 / unsigned 0..255
  - `TYPE_SHORT`: signed -32768..32767 / unsigned 0..65535
  - `TYPE_INT`: signed -2147483648..2147483647 / unsigned 0..4294967295
  - `TYPE_LONG`, `TYPE_LONG_LONG`: always safe for signed; `val >= 0` for unsigned
  - `TYPE_UNSIGNED_LONG_LONG`: `val >= 0`
  - All other types (float, pointer, struct, etc.): returns 0

### Step 2: `src/optimize.c` — `AST_CAST` fold block in `optimize_expr`

- Added fold block between the `AST_SIZEOF_EXPR` block and the `AST_VAR_REF` const-propagation block
- Condition: `node->type == AST_CAST && child_count == 1 && child is AST_INT_LITERAL && is_scalar_int_type(decl_type) && cast_value_safe(val, decl_type, is_unsigned)`
- Action: reads `val` via `strtol`, reads `dst_type` and `dst_unsigned` from the cast node, calls `ast_free(node)` to free cast and child, allocates fresh `AST_INT_LITERAL` with `util_strdup` value string
- Declarations (`buf`, `lit`, `dst_type`, `dst_unsigned`) placed at top of block — C0-compatible

### Step 3: `src/optimize.c` — file-top comment update

- Added Stage 153 comment line after Stage 152 line

### Step 4: Integration tests (5 new)

- `test_cast_fold_basic`: direct cast of integer literals `(int)42L`, `(long)7`, `(int)100LL` → correct typed literals
- `test_cast_fold_sizeof`: `(int)sizeof(long)+1` → 9, `(int)sizeof(int)*2` → 8, `(long)sizeof(char)+7L` → 8
- `test_cast_fold_const_prop`: `const long N = 100; (int)N` — stage-152 substitutes N, stage-153 folds the cast
- `test_cast_fold_dead_branch`: `(int)sizeof(long)==8` → dead-branch elimination via stages 151→153→143→150
- `test_cast_fold_unsafe`: `(char)300` → 44, `(unsigned char)(-1)` → 255 — not folded by optimizer

### Step 5: `src/version.c`

- Bumped `VERSION_STAGE` from `"01520000"` to `"01530000"`

### Step 6: `docs/outlines/checklist.md`

- Added `## Stage 153 - Cast Constant Folding` section with implementation detail bullets
- Flipped `- [ ] Fold through parentheses / AST_CAST` TODO item to `[x]` with `(Stage 153)` annotation

## Final Results

Build: clean (`cmake --build build` — no warnings or errors).
All 2037 portable tests pass (165 unit, 1286 valid, 261 invalid, 154 integration, 50 print-AST, 100 print-tokens, 21 print-asm). No regressions.
Self-host C0→C1→C2 verified: all 2037 tests pass at every stage, no source changes needed during bootstrap.

## Session Flow

1. Read spec (`docs/stages/ClaudeC99-spec-stage-153-cast-constant-folding.md`) and supporting files
2. Established baseline: 2032/2032 portable tests passing at stage 152
3. Produced kickoff artifact (`docs/kickoffs/stage-153-kickoff.md`)
4. Implemented `cast_value_safe` helper and `AST_CAST` fold block in `src/optimize.c`
5. Built and ran smoke test: `(int)sizeof(long)+1` → 9 ✓
6. Verified all 5 new integration tests individually
7. Ran full test suite: 2037/2037 pass
8. Bumped version to `01530000`; updated checklist
9. Committed implementation (ebae392)
10. Ran `./build.sh --mode=self-host`; C0→C1→C2 all 2037 tests pass
11. Updated self-compilation report; committed (f280d2d)
12. Generated milestone and transcript artifacts

## Commit

`ebae392` — stage 153: cast constant folding for scalar integer types
`f280d2d` — docs: stage 153 self-compilation report
