# Stage 39 Kickoff: Minimal const Support

## Summary of the Spec

Stage 39 introduces minimal `const` support to the ClaudeC99 compiler. The implementation will parse the `const` keyword as a type qualifier, store const information on base/pointee types, and enforce rejection of direct assignment to const-qualified scalar variables. Full pointer-level const correctness is deferred to later stages.

Supported patterns:
- `const int x;`
- `const char *s;`
- `const void *p;`
- `int strcmp(const char *a, const char *b);`

Rejected patterns:
- Direct assignment to const scalar variables: `const int x = 3; x = 4;` (rejected)

Out of scope:
- Writes through pointers-to-const: `const char *s; *s = 'x';` (out of scope)
- Conversion enforcement: `const int *` to `int *` (out of scope)
- Read-only segment optimization (out of scope)

## Required Tokenizer Changes

1. Add `TOKEN_CONST` token type to `include/token.h`
2. Update lexer to recognize `"const"` as a keyword in `src/lexer.c`
3. Add const to token display names

## Required Parser Changes

1. Update grammar in `docs/grammar.md`:
   - Add `<type_qualifier> ::= "const"`
   - Extend `<declaration_specifier>` to include `<type_qualifier>`
2. Update `src/parser.c` to handle `TOKEN_CONST` during parsing of:
   - Declaration specifiers
   - Type parsing
   - Parameter declarations
   - Struct member declarations (if applicable)

## Required AST Changes

1. Add `int is_const` field to `ASTNode` struct in `include/ast.h`
2. Ensure const flag propagates through the AST hierarchy during parsing

## Required Code Generation Changes

1. Add `int is_const` field to `LocalVar` struct in `include/codegen.h`
2. Add `int is_const` field to `GlobalVar` struct in `include/codegen.h`
3. Update `src/codegen.c` to:
   - Initialize is_const from AST during codegen setup
   - Propagate const flag through code generation
   - Implement semantic check: reject assignment operations to const-qualified scalar variables
   - Note: Const variables emit identical code to non-const variables; only the semantic check is added

## Test Plan

1. **Valid const declarations** (in `test/valid/`):
   - Simple const scalars: `const int x;`
   - Const pointer declarations: `const char *s;`, `const void *p;`
   - Function parameters with const: `int strcmp(const char *a, const char *b);`
   - Const qualified in various declaration contexts

2. **Invalid const assignments** (in `test/invalid/`):
   - Direct assignment rejection: `const int x = 3; x = 4;`
   - Assignment to const array/struct members where applicable
   - Other direct modification attempts on const variables

## Implementation Order

1. Tokenizer: Add `TOKEN_CONST` and lexer recognition
2. AST: Add `is_const` field to `ASTNode`
3. Headers: Add `is_const` to `LocalVar` and `GlobalVar` in codegen.h
4. Parser: Update grammar and parser to handle const during declaration parsing
5. Codegen: Initialize, propagate, and check is_const; implement assignment rejection semantic check
6. Tests: Add valid and invalid test cases
7. Documentation: Update `docs/grammar.md` with type_qualifier production

## Ambiguities and Decisions

**Ambiguity 1: Pointer-to-const writes**
The spec text shows both "reject `*s = 'x'` on `const char *s`" and "writes through pointers-to-const are out of scope." Interpretation: Follow the Semantic Checking section. Only direct assignment to const scalar variables is rejected; pointer dereference writes are not checked in Stage 39.

**Ambiguity 2: Codegen instruction clarity**
The codegen section contains garbled text: "Store const const in a similar manner as other other of of similar types." Interpretation: Const variables generate identical code to non-const variables; the only addition is the semantic check for assignment rejection.

**Decision: Scope of enforcement**
Only direct assignment to const-qualified lvalues is enforced. This is enforced in codegen during assignment statement processing, not during type conversion.
