# stage-98 Transcript: Compound Literals

## Summary

Stage 98 implements C99 compound literals — unnamed temporary objects created and allocated on the stack with the syntax `(type-name){ initializer-list }`. This enables concise construction of anonymous structs, arrays, and scalars at the point of use, with full support for brace-initializers (including designated initializers from stage 97), postfix operators (`[]`, `.`, `->`), and address-of (`&`). Arrays may have explicit size (e.g., `(int[3]){1,2,3}`) or omitted size (e.g., `(int[]){1,2,3}`), which is inferred from the initializer. Scalars may use braced form (e.g., `(int){42}`). Compound literals can be used as function arguments, in assignments, and their addresses taken for pointer initialization. File-scope compound literals are not yet supported; all are stack-allocated within functions.

The implementation adds the `AST_COMPOUND_LITERAL` node type, extends the parser to detect and construct compound literals at two points (after a cast-style type-name in postfix position, and after `(type-name){` prefix), rewrites the codegen to perform a two-phase pre-scan (compute total stack bytes, then assign offsets), and emits four code patterns depending on the compound literal's type. The compiler self-hosts without issue; all 1521 tests pass.

## Changes Made

### Step 1: AST Node Definition

- Added `AST_COMPOUND_LITERAL` to `ASTNodeType` enum in `include/ast.h` (after `AST_INITIALIZER_LIST`).
- Structure: `decl_type` stores the base kind (e.g., `TYPE_INT`, `TYPE_STRUCT`); `full_type` stores the complete `Type *` (for arrays/structs/unions, or NULL for plain scalars at parse time); `byte_length` initially 0 (assigned by codegen pre-scan to the rbp-relative offset); `value` stores a pre-formatted type label string (unused at runtime, useful for debugging); `children[0]` is either `AST_INITIALIZER_LIST` (for brace form) or a bare expression (scalar braced form, e.g., `(int){42}`).

### Step 2: Parser — `build_compound_literal`

- Implemented new static helper `build_compound_literal(Parser *parser, Type *type)` to construct an `AST_COMPOUND_LITERAL` node from a parsed type and the following brace-initializer.
- For struct/union types: calls `parse_initializer` to consume the brace-list and attach as child.
- For array types with explicit size: calls `parse_initializer` to consume the brace-list.
- For array types with omitted size (`[]`): calls `parse_initializer`, then calls `infer_compound_literal_array_length()` to compute the inferred size from designators and positional elements.
- For scalar types: expects `TOKEN_LBRACE`, parses the single expression (or initializer-list), detaches the `AST_INITIALIZER_LIST` child if present (for `(int){.x = 1}` style, though mostly used as `(int){val}`).
- Returns the constructed `AST_COMPOUND_LITERAL` node.

### Step 3: Parser — `infer_compound_literal_array_length`

- Implemented `infer_compound_literal_array_length(ASTNode *list)` to walk an `AST_INITIALIZER_LIST` and compute the inferred array length.
- Walks all children, tracking a cursor (repositioned by `[N]` designator nodes from stage 97).
- Returns `max(highest designator index + 1, count of non-designated elements)`.
- Handles both `AST_DESIGNATED_INIT` nodes (with array designators; `value == NULL`, index in `byte_length`) and plain initializers.

### Step 4: Parser — `parse_postfix_suffixes`

- Extracted a new helper `parse_postfix_suffixes(Parser *parser, ASTNode *base)` from the existing `parse_postfix` function.
- Applies trailing postfix operators (`[expr]`, `.field`, `->field`, `(args)`, `++`, `--`) to a base expression, returning the final result.
- Used by both the existing `parse_postfix` (for primary expressions) and the new compound-literal path (to apply operators to the literal result).

### Step 5: Parser — `parse_cast` Extended

- After parsing `(type-name)`, check if the next token is `TOKEN_LBRACE`.
- If yes: call `build_compound_literal()` to construct and return the compound literal.
- If no: fall through to existing cast-expression handling.
- Updated type-start detection to include `TOKEN_STRUCT` and `TOKEN_UNION` in the lookahead check (for compound literals like `(struct Point){...}`).

### Step 6: Parser — `parse_postfix` Extended

- Save lexer state before attempting to parse `(type-name)`.
- Look ahead: check if `(type-name){` pattern is confirmed (i.e., `(` followed by type keywords/identifiers and closing `)`).
- If pattern matched: restore state, call `build_compound_literal()`, apply `parse_postfix_suffixes()`, return result.
- If pattern not matched: restore state and fall through to existing `parse_primary` path (for ordinary calls).
- This save/restore enables `&(int){7}` and similar postfix compound-literal construction without disrupting ordinary function calls.

### Step 7: Parser — `parse_type_name` Updated

- Extended `parse_type_name` to accept `[]` (omitted first dimension) in array declarators, used for compound-literal array-size inference.
- Keep error rejection of `[]` for `sizeof(T[])` and cast-expression type-names (check `allow_incomplete_array` context flag).

### Step 8: Parser — `parse_unary` Updated

- Updated address-of (`&`) lvalue check to also accept `AST_COMPOUND_LITERAL` nodes.
- Set result type to `type_pointer(node->full_type->base)` or `type_pointer(node->full_type)` depending on whether the compound literal is a scalar or aggregate.

### Step 9: Parser — Global Initializer Updated

- Updated global initializer path (file-scope variable initialization) to use `parse_assignment_expression` (not `parse_primary`).
- If a compound literal is encountered: emit error "compound literals at file scope are not yet supported".

### Step 10: Codegen — Pre-scan Phase

- Implemented `compute_compound_literal_bytes(ASTNode *expr)`: walks an expression AST, counts all `AST_COMPOUND_LITERAL` nodes encountered, sums their byte sizes (including padding for alignment).
- Implemented `scan_expr_compound_literals(CodeGen *cg, ASTNode *expr)`: second pass, iterates the same expression, assigns each `AST_COMPOUND_LITERAL` a concrete rbp-relative offset stored in `node->byte_length`, incrementing the frame offset counter.
- Both called before the NASM prologue (in `codegen_function`), after all local variable byte requirements are totaled.

### Step 11: Codegen — Compound Literal Emission

- For struct/union compound literals: call `emit_local_struct_init()` with the stack address (rbp-relative offset from `byte_length`).
- For array compound literals: invoke the local array initializer path, using the stack offset and initializer list.
- For scalar compound literals: emit a direct `mov [rbp - offset], value` instruction.
- For address-of struct/union/array: address is already in rax after initialization; for address-of scalar: emit `lea rax, [rbp - offset]`.

### Step 12: Codegen — `emit_struct_addr` Updated

- Updated `emit_struct_addr()` to handle `AST_COMPOUND_LITERAL` nodes.
- Returns `node->full_type->base` (the underlying struct type) instead of `node->full_type` (which codegen may overwrite to a pointer).

### Step 13: Codegen — Address-Of on Compound Literal

- When `AST_ADDR_OF` wraps an `AST_COMPOUND_LITERAL`:
  - Struct/union/array: address already in rax from initialization; set result type to `type_pointer(literal->full_type->base)`.
  - Scalar: emit `lea rax, [rbp - offset]`; set result type to `type_pointer(literal->full_type)`.

### Step 14: Version Bump

- Updated `src/version.c`: `VERSION_STAGE = "00980000"`.

### Step 15: Tests

- **12 valid tests**:
  - `test_compound_literal_struct_arg__7.c` — pass struct compound literal as function argument
  - `test_compound_literal_struct_assign__30.c` — assign struct compound literal to variable
  - `test_compound_literal_array_explicit__10.c` — explicit-size array compound literal
  - `test_compound_literal_array_omitted__18.c` — omitted-size array (inferred from initializer)
  - `test_compound_literal_array_designator__30.c` — array compound literal with index designators
  - `test_compound_literal_postfix_member__99.c` — postfix member access on compound literal
  - `test_compound_literal_postfix_subscript__30.c` — postfix subscript on compound literal
  - `test_compound_literal_addr_of__8.c` — address-of compound literal (`&(int){7}`)
  - `test_compound_literal_scalar__42.c` — scalar compound literal
  - `test_compound_literal_zero_fill__5.c` — zero-fill behavior in compound literal
  - `test_compound_literal_dot_product__23.c` — dot product using array compound literals
  - `test_compound_literal_char_array__2.c` — char array compound literal
- **4 invalid tests**:
  - `test_compound_literal_file_scope__compound_literals_at_file_scope.c` — error for file-scope compound literal
  - `test_compound_literal_void_type__invalid_type_for_compound_literal.c` — void-type literal rejected
  - `test_compound_literal_too_many_init__too_many_initializers.c` — initializer list overflow
  - `test_compound_literal_index_oob__array_designator_index.c` — out-of-bounds array designator
- **3 print_ast tests**:
  - `test_compound_literal_struct.c` + `.expected` — struct compound literal AST output
  - `test_compound_literal_array.c` + `.expected` — array compound literal AST output
  - `test_compound_literal_scalar.c` + `.expected` — scalar compound literal AST output
- **1 integration test**: `test_compound_literal_multifile/` with two source files and compilation/run verification.

### Step 16: Documentation

- Updated `docs/grammar.md`: added `<cast_expression>` rule variant for compound literals.
- Updated `docs/self-compilation-report.md`: added stage 98 section (no issues; 1521/1521 at C0/C1/C2).
- Updated `docs/outlines/checklist.md`: added stage 98 section; flipped compound literals TODO to `[x]`.
- Created `docs/other/stage-98-parse-tree.md`: parse tree snapshot for compound literals.
- Created `status/project-status-through-stage-98.md`: status snapshot.
- Created `status/project-features-through-stage-98.md`: features snapshot.

## Final Results

All 1521 tests pass at all three stages:
- C0 (GCC-built): 1521/1521 pass
- C1 (C0-built, self-host bootstrap): 1521/1521 pass
- C2 (C1-built, second self-host stage): 1521/1521 pass

Breakdown: 855 valid, 246 invalid, 86 integration, 48 print_ast, 100 print_tokens, 21 print_asm.

No regressions. No bootstrap bugs found during self-hosting. The compiler reached a fixed point — C1 and C2 are behavior-identical.

## Session Flow

1. Read spec and supporting template files; reviewed stage 97 transcript and milestone as format references.
2. Analyzed existing cast-expression and postfix-expression machinery.
3. Implemented AST node (`AST_COMPOUND_LITERAL`) with field layout.
4. Implemented `build_compound_literal()` helper to construct literals from parsed types and initializers.
5. Implemented `infer_compound_literal_array_length()` to compute array size from designators.
6. Extracted `parse_postfix_suffixes()` helper for applying operators to both casts and literals.
7. Extended `parse_cast` to detect `(type-name){` and call `build_compound_literal()`.
8. Extended `parse_postfix` with save/restore of lexer state to distinguish `(type-name){` from function calls.
9. Updated `parse_type_name`, `parse_unary`, and global-initializer path to handle compound literals.
10. Implemented two-phase codegen pre-scan: `compute_compound_literal_bytes()` and `scan_expr_compound_literals()`.
11. Implemented codegen emission for struct/union (local-struct-init), array (array init), and scalar (direct store) compound literals.
12. Updated `emit_struct_addr` for correct type handling.
13. Implemented address-of for compound literals with proper type and code paths.
14. Bumped version.
15. Added 20 tests (12 valid, 4 invalid, 3 print_ast, 1 integration); verified pass/fail status.
16. Fixed parser bugs encountered:
    - `build_compound_literal` forward declaration was required.
    - `&(int){7}` parse error fixed by extending address-of lvalue check to include `AST_COMPOUND_LITERAL`.
17. Fixed codegen bugs:
    - "undeclared variable 'int'" codegen error when trying to use type constants; resolved by using `node->byte_length` directly.
    - `emit_struct_addr` returning wrong type for compound literals; fixed to return base type of the struct.
    - File-scope test using `parse_primary` instead of `parse_assignment_expression`; fixed to use correct parser function.
18. Updated grammar and README.
19. Generated documentation artifacts (self-compilation report, parse tree, status/features snapshots).
20. Ran full test suite: all 1521 tests pass.
21. Executed `./build.sh --mode=self-host` for C0→C1→C2 verification: passed cleanly.
22. Committed changes.

## Notes

- **Save/restore in parse_postfix**: The lexer state is saved before attempting to detect `(type-name){` in postfix position. If the pattern does not match, the state is restored and the parser falls through to `parse_primary` for ordinary function calls. This is essential because `(` can start a function call or a compound literal, and the parser must not consume tokens speculatively without being able to undo if the lookahead fails.
- **Omitted array size inference**: Arrays in compound literals may omit the first dimension (e.g., `(int[]){1,2,3}`). The `infer_compound_literal_array_length()` helper walks the initializer list to find the highest designator index or count the plain elements, determining the inferred size. This mirrors stage 97's designated initializer support.
- **Two-phase pre-scan strategy**: Frame offset assignment is deferred to a second pass after all local variable sizes are computed. This ensures no stack collisions and allows the codegen to emit the prologue with the correct frame size before any compound literal initialization code is generated.
- **Scalar compound literals**: Scalar types in braces (e.g., `(int){42}`) are treated as single-element initializers. If an `AST_INITIALIZER_LIST` is encountered (unusual), the first child is extracted and emitted. If a bare expression, it is directly stored to the stack offset.
- **File-scope restriction**: Compound literals at file scope are rejected at parse time with the error "compound literals at file scope are not yet supported". This is a deliberate limitation because stack allocation makes no sense for file-scope objects; support would require either heap allocation or static-storage allocation in data/bss, both of which are out of scope for now.
- **Bootstrap constraints**: All implementation uses C99-compatible patterns and avoids language features unsupported by the C0 compiler. No issues encountered during self-hosting.

## Commit

The stage-98 implementation is captured in a single commit with all changes (parser, codegen, AST, version, tests, docs) and incremental self-host validation.
