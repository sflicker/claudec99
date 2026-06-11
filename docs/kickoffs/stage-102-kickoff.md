# Stage 102 Kickoff — Designated Initializers and Multidimensional Static Arrays

## Summary

Stage 102 closes three codegen gaps deferred from stage 101, all within `src/codegen.c` only. No parser, AST, or grammar changes are required.

The stage enables:

1. **Designated initializers in static local arrays**: `static int arr[4] = {[2] = 99, [0] = 1}`
2. **Static arrays of structs and unions**: `static struct Point pts[3] = {{1, 2}, {3, 4}, {5, 6}}`
3. **Multidimensional static arrays**: `static int grid[2][3]` (both uninitialized in `.bss` and initialized in `.data`)

After this stage, the compiler will have complete coverage for block-scope static array and struct initialization, with support for all scalar and aggregate element types (except nested arrays deeper than 2D), designated indexing, and persistent storage across function calls.

---

## Required Tokenizer Changes

None.

---

## Required Parser Changes

None.

---

## Required AST Changes

None.

---

## Required Code Generation Changes

All changes are in `src/codegen.c` only.

**Task 1: Replace Guard A — designated initializers in static local array `.data` path**

Replace the `compile_error` that rejects `AST_DESIGNATED_INIT` children with index-designator handling identical to the global array path. Distinguish index designators (`value == NULL`, index in `byte_length`) from member designators (`value != NULL`).

**Task 2: Extend slot-emit and zero-fill branches — struct/union element support**

In the slot-emit loop, add branches for struct elements (via `emit_global_struct`), union elements (inline first-member init + zero-fill), and 2D array rows. Extend the NULL-slot zero-fill to handle struct/union element types with byte-granular zeroing.

**Task 3a: Fix `.bss` emission for local static multidimensional arrays**

Replace the current array branch with a two-case check: if the element type is itself an array (multidimensional), use `resb total_bytes`; otherwise use the existing element-directive × length pattern.

**Task 3b: Fix `.bss` emission for file-scope global multidimensional arrays**

Apply the same two-case fix to `codegen_emit_bss`.

**Task 3c: Add `.data` emission for 2D array rows**

In the slot-emit loop, add a branch for inner-dimension rows of multidimensional arrays. Emit each scalar element of the row directly, with zero-fill for missing rows.

**Task 4: Version bump**

Bump `VERSION_STAGE` to `"01020000"` in `src/version.c`.

---

## Test Plan

**Valid tests** (7):
1. `test_block_static_array_designated__0.c` — designated-init with out-of-order indices
2. `test_block_static_array_designated_persist__0.c` — designated-init persists across calls
3. `test_block_static_struct_array_uninitialized__0.c` — uninitialized static array of structs
4. `test_block_static_struct_array_initialized__0.c` — initialized static array of structs
5. `test_block_static_2d_array_uninitialized__0.c` — uninitialized 2D static array (`.bss`)
6. `test_block_static_2d_array_initialized__0.c` — initialized 2D static array (`.data`)
7. `test_block_static_2d_array_persist__0.c` — 2D static array persists across calls

**Invalid tests** (1):
1. `test_block_static_3d_array_initialized__deeper_than_2D.c` — 3D initialized static array, expect "deeper than 2D" error

---

## Implementation Order

1. `src/codegen.c` — Task 1: replace Guard A with designated-init handling
2. `src/codegen.c` — Task 2: extend slot-emit and zero-fill branches for struct/union elements
3. `src/codegen.c` — Task 3a: fix local static multidimensional `.bss` emission
4. `src/codegen.c` — Task 3b: fix global multidimensional `.bss` emission
5. `src/codegen.c` — Task 3c: add 2D array row emission in `.data` loop
6. `src/version.c` — bump stage to `"01020000"`
7. Add tests
8. Update documentation (after all tests pass)

---

## Notes

- This is a codegen-only stage; no tokenizer, parser, or grammar changes.
- All edits are localized to `src/codegen.c` in the static local array initialization loops.
- Out of scope: 3D and deeper static arrays, designated initializers in 2D rows, static arrays of arrays of structs, union elements beyond the first member.
- Bootstrap verification: run `./build.sh --mode=self-host` before declaring the stage complete. The compiler's own source uses no static local arrays of structs or multidimensional static locals, so no source changes are expected.
- No ambiguities found in the spec.
