# stage-12-02 Transcript: Address-of and Dereference

## Summary

Stage 12-02 adds the unary address-of (`&expr`) and dereference
(`*expr`) operators to the compiler, completing the minimum needed to
take a pointer to a variable, store it in a pointer local, and read
through it. Address-of requires an lvalue; the only lvalue available
at this stage is a variable reference, so the parser rejects forms
like `&(x + 1)` and `&5`. Dereference requires a pointer-typed
operand; otherwise a type error is raised at codegen time.

The expression layer now propagates a full `Type *` for pointer
values, allowing nested forms like `**pp` to choose the correct load
width at each dereference. Pointer assignment through `*p`, pointer
arithmetic, and pointer comparisons remain out of scope per the
spec.

## Plan

1. Tokenizer: introduce `TOKEN_AMPERSAND`.
2. AST: add `AST_ADDR_OF` and `AST_DEREF`; reuse `full_type` for
   pointer-valued expression results.
3. Parser: extend `parse_unary` for `&` and `*`; enforce lvalue rule
   for `&` at parse time.
4. Codegen: track per-local kind/full type; emit `lea` for `&` and a
   size-appropriate `mov` for `*`; treat pointer-valued RHS as
   already-in-rax for stores.
5. Pretty printer: render the new node kinds.
6. Tests + grammar update.

## Changes Made

### Step 1: Tokenizer

- Added `TOKEN_AMPERSAND` to `TokenType` in `include/token.h`.
- In `src/lexer.c`, after the existing `&&` check, return
  `TOKEN_AMPERSAND` for a bare `&`.

### Step 2: AST

- Added `AST_ADDR_OF` and `AST_DEREF` to `ASTNodeType` in
  `include/ast.h`.
- Reused the existing `Type *full_type` field on `ASTNode` to carry
  the result Type for pointer-valued expressions.

### Step 3: Parser

- Extended `parse_unary` in `src/parser.c` with two new prefix cases:
  - `TOKEN_AMPERSAND`: build `AST_ADDR_OF`; reject the operand if
    it is not `AST_VAR_REF` ("address-of requires an lvalue").
  - `TOKEN_STAR`: build `AST_DEREF`.
- Updated the function-header comment to reflect the new operator
  set.

### Step 4: Code Generator

- Extended `LocalVar` in `include/codegen.h` with `TypeKind kind`
  and `Type *full_type`.
- Updated `codegen_add_var` to record kind and full Type at
  declaration; both call sites (parameter binding in
  `codegen_function`, declaration in `codegen_statement`) updated.
- Added `local_var_type` helper to recover a `Type *` for a local
  (returns the pointer chain when present, otherwise the
  corresponding integer singleton).
- `AST_VAR_REF` codegen now sets `result_type = TYPE_POINTER` and
  propagates `full_type` for pointer locals.
- `AST_ASSIGNMENT` codegen now treats a pointer RHS the same as a
  long for store widening, and propagates pointer result info.
- New `AST_ADDR_OF` case: emits `lea rax, [rbp - offset]`, sets
  `result_type = TYPE_POINTER`, and constructs
  `full_type = type_pointer(local_var_type(lv))`.
- New `AST_DEREF` case: requires operand `full_type->kind ==
  TYPE_POINTER`, then loads through `[rax]` with the base width
  (`movsx eax, byte`, `movsx eax, word`, `mov eax, dword`, or
  `mov rax, qword`); sets `result_type = base->kind` and
  propagates `full_type` when the base is itself a pointer.
- `AST_DECLARATION` codegen passes the declared kind and full type
  to `codegen_add_var`, and treats a pointer initializer as
  already-in-rax for the size-8 store.

### Step 5: Pretty Printer

- Added `AST_ADDR_OF` ("AddressOf:") and `AST_DEREF`
  ("Dereference:") cases in `src/ast_pretty_printer.c`.

### Step 6: Tests

- `test/valid/test_addr_of_and_deref__7.c`: spec core test, takes
  the address of a local int and reads through the pointer.
- `test/valid/test_assign_through_pointer_var__3.c`: pointer local
  declared without an initializer, then assigned `&x` and read
  through with `*p`.
- `test/valid/test_nested_dereference__9.c`: `int **pp = &p;`
  followed by `return **pp;`.
- `test/invalid/test_invalid_15__cannot_dereference.c`: attempts to
  dereference an `int`.
- `test/invalid/test_invalid_16__requires_an_lvalue.c`: attempts
  `&(x + 1)`.
- `test/print_ast/test_stage_12_02_addr_of_and_deref.c` plus
  matching `.expected` exercising `&` and `*`.

### Step 7: Documentation

- Updated `docs/grammar.md`: `<unary_expression>` now references a
  new `<unary_operator>` non-terminal whose alternatives include
  `*` and `&`.

## Final Results

All 224 tests pass (193 valid + 16 invalid + 15 print_ast),
including the 3 new valid tests, 2 new invalid tests, and 1 new
print_ast test. No regressions.

## Session Flow

1. Read the spec, the prior stage's kickoff/transcript, and the
   relevant compiler source files.
2. Produced the kickoff summary, calling out spec typos and the
   lvalue-rule scope.
3. Added the ampersand token in the lexer and token enum.
4. Added the new AST node kinds and extended the parser for unary
   `&` and `*`.
5. Extended `LocalVar` and codegen with new cases for address-of
   and dereference, plus pointer-aware store/load paths.
6. Updated the pretty printer.
7. Ran the existing test suites to confirm no regressions, then
   added the new valid, invalid, and print_ast tests.
8. Updated `docs/grammar.md` to reflect the expanded unary
   operator set.

## Notes

- The spec's "Nested dereference" example contained typos
  (missing `;`, missing `}`, and `**p` where `p` is `int *`).
  The test was implemented as `return **pp;`, which matches the
  intent.
- The invalid-test runner converts filename underscores to spaces
  for grep matching; error messages that contain hyphens
  (`non-pointer`, `address-of`) cannot be matched as a single
  fragment, so the test filenames use sub-fragments
  (`cannot_dereference`, `requires_an_lvalue`) that hit the
  error message cleanly.
- `Type *full_type` was reused on `ASTNode` for both pointer
  declarations and pointer-valued expression results, avoiding a
  new field while keeping integer code paths unchanged.
