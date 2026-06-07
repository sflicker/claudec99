# Stage 95-07 Kickoff: Convert Remaining Static Usages

## Spec Summary

Stage 95-07 converts the two remaining fixed-capacity items with the lowest blast radius from the fixed-capacity-inventory.md:

1. **`MAX_SWITCH_DEPTH`** (16) — `CodeGen.switch_stack[MAX_SWITCH_DEPTH]` (array of `SwitchCtx` structs). Will be replaced with `Vec switch_stack` in the CodeGen struct. Safe Realloc = YES. The bounds check before push is eliminated; Vec grows dynamically.

2. **`MAX_CALL_LAYOUT_ITEMS`** (24) — Used in `compute_call_layout` in src/codegen.c to index `L->items[i]`. Currently has NO bounds check—silent overflow risk. Will add a compile_error bounds check before indexing. The CallLayout remains a local stack variable (no Vec conversion).

Deferred items (higher blast radius, structural changes required):
- `FUNC_MAX_PARAMS` — embedded array in FuncSig; NO Safe Realloc
- `FUNC_TYPE_MAX_PARAMS` — embedded array in Type; NO Safe Realloc
- `MAX_SWITCH_LABELS` — embedded arrays in SwitchCtx; NO Safe Realloc
- `MAX_USER_LABELS` — 2D char array in CodeGen; NO Safe Realloc
- `MAX_NAME_LEN` — embedded char[] in many structs; widest blast radius
- `MAX_ARRAY_DIMS`, `MAX_INCLUDE_DEPTH`, `MAX_COND_DEPTH` — stack/recursion variables

## Tokenizer Changes

None.

## Parser Changes

None.

## AST Changes

None.

## Code Generation Changes

- `include/codegen.h`: Replace `SwitchCtx switch_stack[MAX_SWITCH_DEPTH]` with `Vec switch_stack` (comment: /* SwitchCtx */)
- `src/codegen.c`:
  - Update `codegen_init` to initialize the new Vec field
  - Update switch statement entry point to use Vec API instead of bounds check
  - Update case/default handler to use Vec API
  - Add compile_error bounds check in `compute_call_layout` before `L->items[i]` indexing
  - Update all read sites of switch_stack to use Vec API
- `include/constants.h`: Remove `MAX_SWITCH_DEPTH` definition
- `src/version.c`: Update `VERSION_STAGE` to "00950700"

## Documentation Changes

- `docs/fixed-capacity-inventory.md`: Mark `MAX_SWITCH_DEPTH` as DONE, update `MAX_CALL_LAYOUT_ITEMS` entry to reflect bounds check addition
- `README.md`: Update through-stage line to reference stage 95-07, remove `MAX_SWITCH_DEPTH` from limits table

## Test Plan

- No new tests are needed; behavior is unchanged
- Run full test suite (1471 tests: 165 unit, 823 valid, 237 invalid, 82 integration, 43 print_ast, 100 print_tokens, 21 print_asm) after each conversion
- Commit and update conversion checklist after each item passes all tests

## Bootstrap Compatibility Notes

The following patterns must be applied when converting to Vec (from fixed-capacity-inventory.md):

1. **Avoid inline pointer dereference in Vec retrieval**: Do not use `*(TypedefName *)vec_get(...)` inline; split to two statements:
   ```c
   T *p = (T *)vec_get(&v, i);
   result = *p;
   ```

2. **Use explicit size_t locals for overflow checks**: Ensure unsigned arithmetic by declaring local `size_t` variables rather than relying on cast-of-member arithmetic:
   ```c
   size_t lim = (size_t)-1;
   size_t cap = v->cap;
   if (cap > lim / 2)
       vec_fatal("capacity overflow");
   ```

3. **Error positions are unreliable during bootstrap failures**: If a bootstrap fails, binary-search by commenting out the second half of suspicious functions to isolate the construct causing the issue.

## Implementation Order

1. Convert `MAX_SWITCH_DEPTH` — lowest blast radius (used at switch entry, case, and default sites in codegen.c)
2. Add bounds check to `MAX_CALL_LAYOUT_ITEMS` — adds compile_error check, no conversion needed

After all items complete, update `docs/fixed-capacity-inventory.md` to mark both items as DONE.

## Ambiguities

None. The spec clearly designates exactly two items for conversion in this stage.

## Decisions

- The `MAX_SWITCH_DEPTH` conversion will use the established Vec module (include/vec.h, src/vec.c)
- The `MAX_CALL_LAYOUT_ITEMS` bounds check will be a compile_error to prevent silent corruption
- Conversions will be performed incrementally (one item at a time) to allow for rapid iteration and isolation of bootstrap failures
