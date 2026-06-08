# ClaudeC99 Stage 95-12 — Fix the `#if` unary-operator overflow and make switch labels dynamic

## Goal

Close the two remaining fixed-capacity issues flagged in
`docs/fixed-capacity-inventory.md` that are *not* on the intentionally-static
list:

1. **`ops[32]` in `eval_cond_unary` (`src/preprocessor.c`)** — an **unchecked**
   stack write (`ops[nops++]` with no bound). A preprocessor `#if` with more
   than 32 chained unary operators (`!`, `-`, `+`, `~`) overruns the buffer; a
   large chain crashes the compiler with SIGSEGV. This is the last unbounded
   fixed-capacity write in the tree.
2. **`MAX_SWITCH_LABELS` (= 256)** — `SwitchCtx.nodes[MAX_SWITCH_LABELS]` and
   `SwitchCtx.labels[MAX_SWITCH_LABELS]` (`include/codegen.h`) impose a hard cap
   on the number of `case`/`default` labels in a single `switch`. Exceeding it
   is a clean `compile_error`, but it is still an arbitrary cap.

After this stage there should be **no unchecked fixed-capacity writes anywhere**
and **no hard cap on case/default labels per switch**.

This continues the 95-xx dynamic-capacity work; reuse the `Vec` / `StrBuf`
modules. Do the two conversions **one at a time**, run the suite (or the
relevant part) after each, and commit before moving on — the same cadence as
stages 95-04 through 95-11.

## Task 1 — preprocessor `#if` unary-operator chain

`eval_cond_unary` collects a run of leading unary operators into a fixed
`char ops[32]` and then applies them right-to-left after evaluating the
primary:

```c
char ops[32];
int  nops = 0;
while (s[*in] == '!' || s[*in] == '-' || s[*in] == '+' || s[*in] == '~') {
    ops[nops++] = s[(*in)++];          /* <-- no bound on nops */
    while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
}
long value = eval_cond_primary(...);
for (int i = nops - 1; i >= 0; i--) { /* apply ops[i] */ }
```

**Reproduction (must crash before the fix, compile after):**

```c
#if !!!!!! ... (>32 of them) ... 1
int x = 5;
#endif
int main(void) { return x; }
```

**Required behavior after the fix:** an arbitrarily long unary-operator chain in
`#if`/`#elif` evaluates correctly with no cap and no overflow. The chain must
still fold right-to-left (operators do not commute: `!-1` ≠ `-!1`).

**Preferred approach** — replace the fixed array with a dynamic `StrBuf` (or a
`Vec` of `char`): append each operator as it is consumed, then iterate the
buffer in reverse to apply them. This keeps the existing structure and stays on
the stage theme. Free the buffer before returning on every path.

**Acceptable alternative** — make `eval_cond_unary` recurse (consume one
operator, recurse for the rest of the unary expression, apply the operator on
the way back). This removes the buffer entirely. Note that a pathological chain
then bounds on C-stack depth rather than a heap buffer; the `StrBuf` approach is
preferred because it has no such practical ceiling.

Do **not** simply add a `nops < 32` bounds check — the goal is to remove the
cap, not to convert a crash into an arbitrary error.

## Task 2 — dynamic switch label tables

`SwitchCtx` (`include/codegen.h`) currently embeds two fixed arrays:

```c
typedef struct {
    ASTNode *nodes[MAX_SWITCH_LABELS];
    int      labels[MAX_SWITCH_LABELS];
    int      count;
    int      default_label;
} SwitchCtx;
```

`SwitchCtx` is itself the element type of `CodeGen.switch_stack`
(`Vec switch_stack; /* SwitchCtx */`, converted in stage 95-07). Replace the two
parallel fixed arrays with dynamic storage so the per-switch label count is
unbounded, then remove `MAX_SWITCH_LABELS` from `include/constants.h`.

**Suggested shape** — convert the two arrays to one `Vec` of a small pair
struct (keeps `nodes[i]`/`labels[i]` aligned by construction):

```c
typedef struct { ASTNode *node; int label; } SwitchLabel;

typedef struct {
    Vec entries;        /* SwitchLabel */
    int default_label;
} SwitchCtx;            /* `count` becomes entries.len */
```

Two parallel `Vec`s are also fine if that is a smaller diff; the pair-struct
avoids keeping two lengths in sync.

**Call sites to update** (all in `src/codegen.c`):

- `collect_switch_labels` — drop both `count >= MAX_SWITCH_LABELS` guards;
  `vec_push` a `SwitchLabel`; `ctx->count` reads become `ctx->entries.len`.
- `switch_lookup_label` — iterate `entries` via `vec_get`.
- The `AST_SWITCH_STATEMENT` handler — the dispatch loop reads `ctx->nodes[i]` /
  `ctx->labels[i]`; read through `vec_get` instead.

**Lifecycle — this is the important part.** `SwitchCtx` is stored *by value*
inside `switch_stack`, so its embedded `Vec` is a struct-in-struct-in-Vec:

- When a switch begins, the handler builds a `SwitchCtx new_ctx`, then
  `vec_push(&cg->switch_stack, &new_ctx)`. **Init `entries` and set
  `default_label` before the push** (the push bit-copies the struct, including
  the `Vec`'s data pointer, into the stack slot — a move; the local `new_ctx`
  must not also free it).
- After the push, get the live element with
  `vec_get(&cg->switch_stack, cg->switch_depth - 1)` and use that pointer for
  `collect_switch_labels` and the dispatch loop.
- **Before `vec_pop(&cg->switch_stack)`**, `vec_free(&ctx->entries)` so the
  inner buffer is released (otherwise every switch leaks its label table).

Confirm `vec_pop` does not need the element afterward (it just decrements
`len`), so freeing the inner `Vec` first is safe.

## Out of scope — intentionally-static limits (do NOT convert)

By project decision the following remain fixed and must be left alone. This
stage should not touch them, and the README documents them as permanent with
their current values:

| Constant | Value | Where |
|----------|-------|-------|
| `FUNC_MAX_PARAMS` | 16 | `include/constants.h`; `FuncSig.param_types[]` |
| `FUNC_TYPE_MAX_PARAMS` | 16 | `include/constants.h`; `Type.params[]` |
| `MAX_ARRAY_DIMS` | 8 | `src/parser.c` (local `#define`) |
| `MAX_INCLUDE_DEPTH` | 64 | `include/constants.h` (recursion-depth guard) |
| `MAX_COND_DEPTH` | 64 | `include/constants.h`; `cond_stack[]` |

(`MAX_CALL_LAYOUT_ITEMS` = 24 also stays — it is a derived bound on a local
stack struct and already has a `compile_error` guard from stage 95-07.)

## Bootstrap caveats

The compiler must continue to self-host (C0→C1→C2). Watch for the two patterns
documented in `docs/fixed-capacity-inventory.md` whenever you touch `Vec` code:

- **C0 cannot parse `*(T **)vec_get(...)`** — split a cast-of-dereference into
  two statements (`T **p = (T **)vec_get(...); T *e = *p;`).
- **Signed division in capacity guards** — any `cap > LIMIT / 2` style check
  must use explicit local `size_t` operands so the C0 backend emits unsigned
  `div`, not `idiv`. (`Vec`/`StrBuf` already follow this; only relevant if you
  extend them.)

Reproduce suspicious constructs in a small file against `build/ccompiler-c0`
before running a full `./build.sh --mode=self-host` cycle.

## Tests

- **Preprocessor overflow (Task 1):** add a `test/valid/` case with a long
  chain of `#if` unary operators (e.g. 100+ leading `!`/`~`) guarding a value,
  asserting the folded result via the program exit code. This both proves the
  overflow is gone and pins the right-to-left folding semantics. A focused
  `test/integration/` case is also acceptable.
- **Switch labels (Task 2):** add a `test/valid/` case with a `switch` that has
  **more than 256** `case` labels (generate it if needed) and verify it
  compiles, assembles, links, and selects the correct arm at runtime.
- Update any existing test or golden file that referenced the old cap.

## Docs (at stage close, after all tests pass)

- `docs/fixed-capacity-inventory.md` — mark the `ops[]` buffer and
  `MAX_SWITCH_LABELS` ✓ DONE (95-12); update the "Open issue" callout and the
  Summary so no unchecked/uncapped items remain.
- `README.md` — remove `MAX_SWITCH_LABELS` from the Compiler-limits tables; keep
  the intentionally-static set documented (see above).
- Run the `update-supplemental-docs` skill to refresh the checklist, parse-tree,
  and status/features snapshots through stage 95-12.
- Update `src/version.c` (`VERSION_STAGE "00951200"`).
