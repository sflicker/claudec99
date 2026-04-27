# stage-13-02 Transcript: Array Indexing

## Summary

Stage 13-02 introduces the array subscript expression `a[i]` to the
compiler, both as an rvalue (read) and as an assignment target
(lvalue). It builds directly on stage 13-01, which added the array
type and array declarations but rejected any use of an array name as
a value. With this stage, the only legal use of an array name is now
exactly that: as the base of a `[]` subscript.

The implementation parses the subscript suffix as a new branch of the
postfix expression rule, adds an `AST_ARRAY_INDEX` node carrying the
base and the index expression, and emits address arithmetic of the
form `lea` + sign-extended index times element size. Reads load
through the address using the element type's width with sign
extension for char/short; writes store through the address with the
element width after converting the RHS to the element type.

Pointer indexing, array-to-pointer decay, passing arrays to
functions, pointer arithmetic, and multidimensional arrays are out
of scope this stage and remain rejected.

## Plan

The plan called for changes in this order: AST node, parser,
codegen, pretty printer, grammar, tests. The one spec issue noted
during kickoff was a missing leading double-quote in the spec's
grammar block (`| ++"`), which was read charitably as `"++"`.

## Changes Made

### Step 1: AST

- Added `AST_ARRAY_INDEX` to `ASTNodeType` in `include/ast.h`.
- Layout: `children[0]` = base (an `AST_VAR_REF`),
  `children[1]` = index expression.

### Step 2: Parser

- Extended `parse_postfix` in `src/parser.c` to accept
  `TOKEN_LBRACKET` after a primary: parse the inner expression,
  expect `]`, build `AST_ARRAY_INDEX`. The base must be
  `AST_VAR_REF`; non-identifier bases are rejected.
- Added `AST_ARRAY_INDEX` to the assignment-LHS lvalue check in
  `parse_expression` alongside `AST_DEREF` and `AST_VAR_REF`.

### Step 3: Code Generator

- Added `emit_array_index_addr` helper that emits the element
  address into `rax` and returns the element `Type *`. Validates
  that the base local has `kind == TYPE_ARRAY` and that the index
  is integer-typed; sign-extends int indices via `movsxd` and
  scales by the element size with `imul` (skipping the multiply
  when element size is 1).
- Added a forward declaration for `codegen_expression` so the
  helper can be defined before it.
- Added an `AST_ARRAY_INDEX` rvalue path in `codegen_expression`:
  computes the address, then loads through it at the element
  width — `movsx eax, byte [rax]` / `movsx eax, word [rax]` /
  `mov eax, [rax]` / `mov rax, [rax]`.
- Added an `AST_ARRAY_INDEX` LHS path in `AST_ASSIGNMENT`:
  computes the address, pushes it, evaluates the RHS, converts
  it to the element type via `emit_convert`, pops the address
  into `rbx`, and stores at the element width.
- Extended `expr_result_type` with an `AST_ARRAY_INDEX` case that
  returns the (promoted) element type so binary arithmetic over
  `a[i]` operands picks the correct common type.

### Step 4: Pretty Printer

- Added `case AST_ARRAY_INDEX: printf("ArrayIndex:\n");` in
  `src/ast_pretty_printer.c`. Default child recursion handles
  printing the base and index.

### Step 5: Grammar

- Updated `<postfix_expression>` in `docs/grammar.md` to include
  the `"[" <expression> "]"` alternative.

### Step 6: Tests

- `test/valid/test_array_index_int_3__15.c` — `int a[3]` reads
  and writes summing to 15.
- `test/valid/test_array_index_char_2__30.c` — `char a[2]`
  reads and writes summing to 30.
- `test/valid/test_array_index_long_2__42.c` — `long a[2]`
  reads and writes summing to 42.
- `test/print_ast/test_stage_13_02_array_index.c` and
  `.expected` — verifies `ArrayIndex:` rendering plus children
  layout for both a write (assignment LHS) and a read (return).

## Final Results

All 271 tests pass: 217 valid + 35 invalid + 19 print_ast. No
regressions.

## Session Flow

1. Read spec, supporting skill files, prior milestone, and
   relevant existing source.
2. Confirmed array infrastructure from stage 13-01 (types, lexer,
   declaration parsing, declaration codegen) was already in place.
3. Produced kickoff summary, called out spec grammar typo.
4. Implemented AST node, parser changes, codegen helper and
   rvalue/lvalue paths, pretty printer, grammar update.
5. Built and ran new spec-derived tests in isolation, then ran
   the full valid/invalid/print_ast suites — no regressions.
6. Wrote milestone summary and this transcript.

## Notes

- The address helper uses `lea` + sign-extended `imul` rather
  than `lea [rbp + index*scale]` to avoid restrictions on the
  scale factor — `int` arrays scale by 4, but `long` arrays
  scale by 8 and char arrays skip the multiply entirely
  (handled with an explicit branch on `elem_size != 1`).
- Pointer-element arrays are not exercised by the spec tests
  but the codegen paths use the element type's `size` and
  `kind`, so a pointer element would naturally take the 8-byte
  load/store path. Compatibility checking on RHS for pointer-
  element assignment is intentionally not added — it remains a
  candidate for a later stage along with pointer indexing.
