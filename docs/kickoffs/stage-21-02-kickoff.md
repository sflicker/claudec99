# Stage 21-02 Kickoff: Separate Function Definitions from Declarations

## Spec Summary

Refactor the external declaration parser to separate function definitions from declarations as two distinct grammar paths. Currently `parse_function_decl` in `src/parser.c` handles both a function definition (type_specifier + function_declarator + block_statement) and a function prototype (type_specifier + function_declarator + ";") as a single production. After this stage the two paths are explicit in `parse_external_declaration`.

### Key outcomes:
- Function definitions parse through explicit `<function_definition>` path (type + declarator + block)
- Function prototypes parse through `<declaration>` path (type + declarator + ";")
- Forbid initializers on function declarations (e.g., `int f() = 3;` is an error)
- File-scope object declarations produce AST_DECLARATION nodes (silently ignored by codegen)

## Tokenizer Changes
None required.

## Parser Changes

### Current state
- `parse_external_declaration` calls `parse_function_decl`
- `parse_function_decl` handles both definitions (+ block) and prototypes (+ ";")

### Required changes
Refactor `parse_external_declaration` with explicit dispatch:

1. Parse type_specifier and declarator
2. Examine next token to determine path:
   - `{` → function definition path (requires declarator resolves to function type)
   - `=` → error: "function declaration cannot have an initializer"
   - `;` → prototype/file-scope object declaration path
3. For non-function declarator + `{` → error: "not a function declarator" (preserves test_invalid_105)
4. Fold `parse_function_decl` into `parse_external_declaration`

## AST Changes
None required.
- Function definitions: AST_FUNCTION_DECL with body (unchanged)
- Function prototypes: AST_FUNCTION_DECL without body (unchanged)
- File-scope object declarations: AST_DECLARATION nodes (codegen ignores)

## Code Generation Changes
None required.

## Test Plan

### Valid tests to add
- `test_proto_identity_ptr__7.c` — pointer-return-type prototype + definition (from spec example)

### Invalid tests to add
- `test_invalid_106__function_declaration_cannot_have_an_initializer.c` — `int f() = 3;`

### Existing tests
- `test_invalid_105` covers: non-function declarator + `{` (preserved behavior)
- All function definition and prototype tests remain valid

## Implementation Order

1. **Parser** — refactor `parse_external_declaration` to dispatch on next token
2. **Tests** — add valid and invalid test cases
3. **Documentation** — update docs/grammar.md with new grammar rules
4. **README** — update project status

## Grammar Corrections Noted

The spec contains three grammar corrections needed in docs/grammar.md:
1. Line 12: `<function>` should be `<function_definition>`
2. Line 23: `<declaration> ::-` typo, should be `::=`
3. Line 25: `<init_declaration_list>` should be `<init_declarator_list>`

## Ambiguities and Decisions

**Question: How to handle duplicate prototypes?**
- Decision: Allow multiple identical prototypes (common C pattern)
- Existing behavior is preserved; stage 21-01 already forbids duplicate definitions

**Question: Should file-scope object declarations be validated?**
- Decision: No — codegen ignores them, parser accepts and discards AST_DECLARATION nodes
- Out of scope: storage class, linkage, multiple declarators in one declaration

**Question: How to test the new `=` error without making a C file?**
- Decision: `test_invalid_106__function_declaration_cannot_have_an_initializer.c` contains invalid C
- Parser rejects during external declaration parsing before codegen runs
