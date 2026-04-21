# Stage-11-02 Kickoff: Integer Type Codegen

## Spec Summary

Extend codegen so local variables declared with `char`, `short`, `int`,
`long` are stored with their proper size (1/2/4/8 bytes) and alignment.
All integer types are treated as signed. Storing a larger value into a
smaller variable truncates. Integer promotions, usual arithmetic
conversions, unsigned behavior, mixed-type expression rules, casts,
function parameter passing changes, and return-type widening/narrowing
remain out of scope for this stage.

## Diff vs Previous Stage

Stage 11-01 introduced `TypeKind` on AST_DECLARATION nodes but left
codegen untouched: every local variable was allocated a 4-byte slot
and loaded/stored through `eax`. Stage 11-02 must now:

1. Allocate stack slots sized to the variable's declared `TypeKind`,
   respecting alignment equal to size.
2. Load variables at their correct width (sign-extend `char`/`short` to
   32 bits; read `int` at 32 bits; for `long`, read the low 32 bits into
   `eax` — no 64-bit arithmetic in this stage).
3. Store truncated to the variable's declared size (`al` / `ax` /
   `eax` / `rax`).
4. Keep parameters (still `int`-only) and return values unchanged.

## Ambiguity / Inconsistency Check

The spec says "larger variables stored in smaller ones will be
truncated" but explicitly excludes integer promotions and mixed-type
expression rules. For `long`, the only stage-consistent reading is:
arithmetic continues in `eax` (32-bit), so `long` loads take the low
32 bits and stores sign-extend `eax` to `rax`. This matches the
exclusion list and keeps the change minimal. No further contradictions
in the spec.

## Planned Changes

- **codegen.h** — add `int size;` to `LocalVar`.
- **codegen.c**
  - Map `TypeKind` → size (1/2/4/8).
  - `codegen_add_var(name, size)`: align stack_offset to `size` before
    allocating; record the size in the local table.
  - Change `codegen_find_var` to return the `LocalVar*` (or both offset
    and size) so load/store can emit the correct width.
  - Width-appropriate emit for `AST_VAR_REF`, `AST_ASSIGNMENT`,
    `AST_PREFIX_INC_DEC`, `AST_POSTFIX_INC_DEC`, and the initializer
    path of `AST_DECLARATION`.
  - Replace `count_declarations`-based stack sizing with a size-aware
    walk (conservative: 8 bytes per declaration, rounded to 16).
- **Tests** — add valid-exit-code tests covering: truncation of int into
  char/short, stored `long` persistence through an assignment, and
  load/store of each integer type in an expression.
- **docs/grammar.md** — unchanged (no grammar change this stage).

## Step Plan

1. Codegen header & local-var size metadata.
2. Codegen size-aware allocator, loader, storer.
3. Inc/dec and declaration-initializer width adjustments.
4. Stack-size calculation update.
5. Add new valid tests.
6. Build & run full test suite.
7. Write milestone summary and transcript; commit.
