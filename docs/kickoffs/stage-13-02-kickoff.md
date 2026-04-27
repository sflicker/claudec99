# Stage-13-02 Kickoff: Array Indexing

## Spec summary

Stage-13-02 adds the array subscript expression `a[i]`, both as a read
(rvalue) and as an assignment target (lvalue). Stage 13-01 introduced
array declarations and array Type entries; this stage gives them their
first use beyond declaration.

Semantics:
- `a[i]` is treated as `*(base_address(a) + i * sizeof(element_type))`.
- The result is an lvalue of the array's element type.
- `i` must be an integer-typed expression.
- The base of `[]` must be an array type.

Out of scope (per spec):
- Pointer indexing `p[i]`
- Array-to-pointer decay
- Passing arrays to functions
- Pointer arithmetic
- Multidimensional arrays

## What changes from stage 13-01

Stage 13-01 added array declarations and codegen-time allocation, but
codegen still rejects any use of an array name as a value
(`AST_VAR_REF` of an array local) and rejects assignment to one. This
stage introduces the only legal use of an array name today: as the
base of a `[]` subscript. The base sub-expression in an
`AST_ARRAY_INDEX` is therefore *not* loaded as a value — codegen
takes its address (`lea`) and reads/writes through it.

## Spec issues / ambiguity called out

1. The grammar block in the spec has a typo: `| ++"` is missing a
   leading double-quote. Charitable reading: `"++"`. The existing
   postfix `++` / `--` branches are unaffected; subscript is added
   as a third alternative.
2. The three example C programs in the spec do not encode their
   expected exit code in their filenames — the project's convention
   is `test_<name>__<exit>.c`. I will use the comment-stated values
   (15, 30, 42) as the suffix.
3. Index widening to 64 bits is unspecified in the address-formula
   sentence. I will sign-extend the int index to 64 bits before
   multiplying by `sizeof(element)`, matching the rest of the
   existing 64-bit address arithmetic.

## Planned changes

### Tokenizer
- No changes. `TOKEN_LBRACKET` / `TOKEN_RBRACKET` already exist
  (added stage 13-01).

### AST
- New `ASTNodeType` value `AST_ARRAY_INDEX` in `include/ast.h`.
- Layout: `children[0]` = base (`AST_VAR_REF` of an array local),
  `children[1]` = index expression.

### Parser (`src/parser.c`)
- Extend `parse_postfix` to accept `[` after a primary: parse the
  inner expression, expect `]`, wrap into an `AST_ARRAY_INDEX` node.
  The loop continues so chained postfix forms (e.g. `a[i]++` is out
  of scope this stage) remain parseable in syntax shape.
- Allow `AST_ARRAY_INDEX` as a permitted lvalue in
  `parse_expression`'s assignment LHS check (currently accepts
  `AST_DEREF` and `AST_VAR_REF`).

### Codegen (`src/codegen.c`)
- New rvalue path for `AST_ARRAY_INDEX`:
  - Look up the base local; require `kind == TYPE_ARRAY`. (Out of
    scope: pointer indexing — reject any non-array base.)
  - Validate the index expression's type is integer
    (`type_is_integer`).
  - Emit address computation:
    `lea rax, [rbp - base_offset]`, push,
    evaluate index into `eax`, sign-extend to `rax`,
    multiply by element size, pop base into `rbx`,
    `add rax, rbx`.
  - Load the element using the element type's width with sign
    extension into `eax`/`rax`.
  - Set `node->result_type` from element type; for pointer-element
    arrays, also set `node->full_type`.
- Extend `AST_ASSIGNMENT` to handle `AST_ARRAY_INDEX` as LHS:
  - Compute the same address; push it.
  - Evaluate RHS; convert to element type with `emit_convert`.
  - Pop address into `rbx`; store at the element's width.
- Update `expr_result_type` to recognize `AST_ARRAY_INDEX` so
  expressions like `a[0] + a[1]` get the correct common type.

### Pretty printer (`src/ast_pretty_printer.c`)
- Render `AST_ARRAY_INDEX` as `ArrayIndex:` with default child
  recursion so children are indented under it.

### Grammar (`docs/grammar.md`)
- Update `<postfix_expression>` to:
  `<primary_expression> { "[" <expression> "]" | "++" | "--" }`.

### Tests
- `test/valid/test_array_index_int_3__15.c` — spec test 1.
- `test/valid/test_array_index_char_2__30.c` — spec test 2.
- `test/valid/test_array_index_long_2__42.c` — spec test 3.
- `test/print_ast/test_stage_13_02_array_index.c` plus expected.

### Commit
- Stage all source/header/grammar/test changes.
- Commit message: `stage-13-02: array indexing`.
