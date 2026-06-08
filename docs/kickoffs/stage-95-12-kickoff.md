# stage-95-12 Kickoff — Fix `#if` unary-operator overflow and make switch labels dynamic

## Summary of the spec

Close the two remaining fixed-capacity issues:

1. **`ops[32]` in `eval_cond_unary` (`src/preprocessor.c`)** — unchecked stack write (`ops[nops++]` with no bound). A preprocessor `#if` with >32 chained unary operators crashes with SIGSEGV.
2. **`MAX_SWITCH_LABELS` (= 256)** — hard cap on case/default labels per switch in `SwitchCtx`. After this stage, there should be no unchecked fixed-capacity writes and no hard cap on case/default labels per switch.

Approach: replace both with dynamic storage (Task 1: `StrBuf` for operators; Task 2: `Vec` of pair struct for labels). Run tests and commit after each task.

## Required tokenizer changes

None.

## Required parser changes

None.

## Required AST changes

None.

## Required code generation changes

### Task 1: Replace `ops[32]` with dynamic `StrBuf`

In `src/preprocessor.c`:
- Replace the fixed `char ops[32]` in `eval_cond_unary` with `StrBuf ops; strbuf_init(&ops);`
- Append each unary operator as consumed: `strbuf_append_char(&ops, s[(*in)++]);`
- Iterate the buffer in reverse to apply operators (preserving right-to-left folding)
- Free the buffer on every return path: `strbuf_free(&ops);`
- Add `#include "strbuf.h"` to the header section

### Task 2: Make switch labels dynamic

In `include/codegen.h`:
- Add typedef: `typedef struct { ASTNode *node; int label; } SwitchLabel;`
- Replace `SwitchCtx`'s parallel fixed arrays with: `Vec entries; /* SwitchLabel */` and `int default_label`
- `count` becomes `entries.len`

In `src/codegen.c`, update call sites:
- **`collect_switch_labels`** — drop both `>= MAX_SWITCH_LABELS` guards; `vec_push` a `SwitchLabel` struct; replace `ctx->count` reads with `ctx->entries.len`
- **`switch_lookup_label`** — iterate `entries` via `vec_get` instead of direct array access
- **`AST_SWITCH_STATEMENT` handler** — replace dispatch loop array reads (`ctx->nodes[i]`, `ctx->labels[i]`) with `vec_get` calls

### Lifecycle management for `SwitchCtx` (critical)

`SwitchCtx` is stored *by value* inside `switch_stack` Vec, so its embedded `Vec` requires careful move semantics:

- **Before `vec_push(&cg->switch_stack, &new_ctx)`**, init `entries` and set `default_label` (the push bit-copies the struct; the local `new_ctx` must not also free it)
- After push, get the live element: `vec_get(&cg->switch_stack, cg->switch_depth - 1)` for all `collect_switch_labels` and dispatch loop uses
- **Before `vec_pop(&cg->switch_stack)`**, call `vec_free(&ctx->entries)` to release the inner buffer (otherwise every switch leaks its label table)

In `include/constants.h`, remove `MAX_SWITCH_LABELS` definition.

## Bootstrap caveats

- **C0 cannot parse `*(T **)vec_get(...)`** — split cast-of-dereference into two statements: `T **p = (T **)vec_get(...); T *e = *p;`
- **Signed division in capacity guards** — use explicit `size_t` operands for `cap > LIMIT / 2` style checks so C0 emits unsigned `div`, not `idiv`

Reproduce suspicious constructs against `build/ccompiler-c0` before running full self-host cycle.

## Test plan

**Task 1 (Preprocessor overflow):**
- Add `test/valid/` case with 100+ chained unary operators (`!` or `~`) in `#if` expression guarding a value
- Assert folded result via program exit code
- Proves overflow is gone and pins right-to-left folding semantics

**Task 2 (Switch labels):**
- Add `test/valid/` case with `switch` having >256 `case` labels
- Verify it compiles, assembles, links, and selects the correct arm at runtime

## Out of scope — intentionally-static limits (do NOT touch)

The following remain fixed-capacity and are NOT converted in this stage:

- `FUNC_MAX_PARAMS` (16)
- `FUNC_TYPE_MAX_PARAMS` (16)
- `MAX_ARRAY_DIMS` (8)
- `MAX_INCLUDE_DEPTH` (64)
- `MAX_COND_DEPTH` (64)
- `MAX_CALL_LAYOUT_ITEMS` (24)

## Docs at stage close

After all tests pass:
- `docs/fixed-capacity-inventory.md` — mark `ops[]` and `MAX_SWITCH_LABELS` ✓ DONE (95-12); update Open issue callout and Summary
- `README.md` — remove `MAX_SWITCH_LABELS` from Compiler-limits tables; keep intentionally-static set documented
- Run `update-supplemental-docs` skill to refresh checklists and status through stage 95-12
- Update `src/version.c` — `VERSION_STAGE "00951200"`
