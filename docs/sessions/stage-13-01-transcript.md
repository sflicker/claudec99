# stage-13-01 Transcript: Array Types

## Summary

Stage 13-01 introduces array declarations as a new local-variable
kind. The type system gains a `TYPE_ARRAY` kind that records the
element type, the element count, and the total byte size. Local
declarations of the form `<type> <id> "[" <integer_literal> "]" ";"`
are accepted for any existing scalar element type and for
pointer-to-scalar element types (e.g. `int a[3];`, `char a[4];`,
`int *ptrs[2];`). Codegen allocates `sizeof(element) * length` bytes
on the stack and aligns the slot to the element's natural alignment;
the locals table tracks arrays the same way scalars are tracked,
just with a larger size.

The size must be a positive integer literal; `a[0]`, `a[-1]`, and
`a[x]` are rejected at parse time. Array initializers, array
indexing, array-to-pointer decay, pointer arithmetic,
multi-dimensional arrays, and global arrays are all out of scope and
not implemented. To match the "arrays are not assignable" rule,
codegen rejects assignment to an array local and rejects any other
use of an array name as a value (the spec test surface never reads
array variables).

## Plan

1. Pause to flag spec issues: the grammar tension between
   `[ "=" <expression> ]` and the out-of-scope "Array initializers"
   list; the `int a[-1]` rejection mechanism; the alignment subtlety
   for non-power-of-two array sizes; and the silence on array reads.
2. Implementation order: tokenizer → type system → parser → codegen
   → pretty printer → tests → grammar doc.
3. Resolve the grammar/initializer tension by rejecting `=` after
   array brackets in the parser. Use an explicit `align` parameter
   in `codegen_add_var` so existing scalars are unchanged.

## Changes Made

### Step 1: Tokenizer

- Added `TOKEN_LBRACKET` and `TOKEN_RBRACKET` to the `TokenType`
  enum.
- Lexed `[` and `]` as single-character tokens in
  `lexer_next_token`.

### Step 2: Type System

- Added `TYPE_ARRAY` to the `TypeKind` enum.
- Added an `int length` field to `Type` (used only for arrays).
- Added `Type *type_array(Type *element_type, int length)`,
  heap-allocated like `type_pointer`, recording total `size`,
  element `alignment`, and element type in `base`.
- Extended `type_kind_name` and `type_is_integer` for `TYPE_ARRAY`.
- Updated existing scalar singletons to initialize the new
  `length` field to `0`.

### Step 3: Parser

- In `parse_statement`'s declaration branch, after the identifier,
  optionally consume `"[" <integer_literal> "]"`.
- Reject when the bracketed token is not `TOKEN_INT_LITERAL`
  ("array size must be an integer literal").
- Reject when the parsed length is `<= 0` ("array size must be
  greater than zero").
- Mark the declaration with `decl_type = TYPE_ARRAY` and
  `full_type = type_array(element_type, length)` where the element
  type is the (possibly pointer-wrapped) base type.
- Reject `=` after the closing bracket ("array initializers not
  supported").

### Step 4: Code Generator

- `type_kind_bytes(TYPE_ARRAY)` returns 0; the actual size lives
  on the array Type's `size` field.
- Added an `int align` parameter to `codegen_add_var`. The stack
  bump uses `size`; the alignment mask uses `align`. Existing
  scalar/pointer call sites pass `align = size` (unchanged
  behavior).
- Updated `compute_decl_bytes` to account for the actual array
  byte count plus 7 bytes of alignment slack per array
  declaration (scalars/pointers continue to use the 8-byte
  conservative bound).
- `AST_DECLARATION` for `TYPE_ARRAY` allocates the slot via
  `codegen_add_var(name, full_type->size, full_type->base->alignment, ...)`
  and returns immediately (the parser has rejected any
  initializer).
- `AST_VAR_REF` of an array local now errors ("cannot use array
  variable '<name>' as a value").
- `AST_ASSIGNMENT` whose LHS local is an array now errors
  ("arrays are not assignable").

### Step 5: Pretty Printer

- `AST_DECLARATION` with `decl_type == TYPE_ARRAY` renders the
  element type (which may itself be a pointer chain) followed by
  `<name>[<length>]`.

### Step 6: Tests

- New valid tests in `test/valid/`:
  - `test_array_int_3__0.c`
  - `test_array_mixed_locals__0.c`
  - `test_array_char_long__0.c`
  - `test_array_short__0.c`
  - `test_array_of_pointers__0.c`
- New invalid tests in `test/invalid/`:
  - `test_invalid_32__array_size_must_be_greater_than_zero.c`
  - `test_invalid_33__array_size_must_be_an_integer_literal.c`
    (covers `int a[-1];`)
  - `test_invalid_34__array_size_must_be_an_integer_literal.c`
    (covers `int a[x];`)
  - `test_invalid_35__arrays_are_not_assignable.c`
  - `test_invalid_36__array_initializers_not_supported.c`
- New print-AST test:
  - `test_stage_13_01_arrays.c` / `.expected` — covers `int a[3];`,
    `char c[4];`, and `int *ptrs[2];`.

### Step 7: Grammar Doc

- Updated `docs/grammar.md` `<declaration>` to:
  `<type> <identifier> [ "[" <integer_literal> "]" ] [ "=" <expression> ] ";"`

## Final Results

All 267 tests pass (214 valid + 35 invalid + 18 print-AST). No
regressions.

## Session Flow

1. Read the stage 13-01 spec and supporting files.
2. Reviewed the current lexer, parser, type system, codegen, and
   pretty printer to confirm where the array changes land.
3. Drafted a kickoff document calling out the grammar/initializer
   tension and the alignment subtlety.
4. Implemented the changes in small steps: tokenizer, type system,
   parser, codegen, pretty printer.
5. Built the compiler; ran valid, invalid, and print-AST suites to
   confirm no regressions.
6. Added the 5 valid + 5 invalid + 1 print-AST tests and re-ran
   all suites.
7. Updated `docs/grammar.md` for the new `<declaration>` production.
8. Wrote the milestone summary and this transcript.

## Notes

- Alignment: the prior `codegen_add_var` aligned to `size`, which
  works for `size ∈ {1,2,4,8}` but not for arrays whose total byte
  count is not a power of two (e.g. `int[3]` is 12 bytes, but its
  natural alignment is 4). Splitting `align` from `size` keeps
  scalars unchanged and gives arrays element-aligned slots.
- The `int a[-1]` rejection rides on `-` lexing as a separate
  token, so the size production sees a non-`INT_LITERAL` token and
  errors with "array size must be an integer literal" — the same
  fragment used for `int a[x]`.
- Out of scope per spec: array indexing, pointer arithmetic,
  array-to-pointer decay, array initializers, multi-dimensional
  arrays, global arrays. None of these are implemented this stage.
