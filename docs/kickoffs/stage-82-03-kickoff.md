# Stage 82-03 Kickoff: const in type-name contexts

## Summary of the spec

Allow `const` (and `volatile`) type qualifiers inside type-name contexts where the parser currently rejects them:

- `sizeof(const char *)` — sizeof with qualified type and pointer
- `sizeof(const int)` — sizeof with qualified base type
- `(const char *)p` — cast to qualified pointer type
- `va_arg(ap, const char *)` — va_arg with qualified type

Currently, the parser fails on these because the lookahead checks for `sizeof` and cast expressions do not include `TOKEN_CONST`, so the parser never enters the type-name parsing path.

The grammar change aligns type-name production with declaration specifiers to allow qualifiers before the optional declarator.

## Tokenizer changes

None. TOKEN_CONST already exists and is properly recognized.

## Parser changes

Three targeted changes to `/src/parser.c`:

1. **sizeof lookahead** (around line 1472):
   - Add `TOKEN_CONST` to the condition that detects `sizeof(type-name ...)`
   - Currently checks for `type_specifier_tokens` only

2. **cast expression lookahead** (around line 1563):
   - Add `TOKEN_CONST` to the condition that detects `(type-name) ...`
   - Currently checks for `type_specifier_tokens` only

3. **parse_type_name function**:
   - Call `parse_specifier_qualifier_list()` instead of `parse_declaration_specifiers()`
   - Handle `const` qualifiers after `*` in abstract declarator
   - Update abstract declarator parsing to support type qualifiers on pointer declarators

## AST changes

None. The existing `struct ast_declarator` and qualifier fields support qualified pointers.

## Code generation changes

None. Type qualifiers are semantically irrelevant for code generation in this context.

## Test plan

Three new integration tests in `test/valid/`:

1. **sizeof with qualified base type**
   - Test: `sizeof(const int) == sizeof(int)`
   - Validates that const does not affect size

2. **sizeof with qualified pointer type**
   - Test: `sizeof(const char *) == sizeof(char *)`
   - Validates that const on pointed-to type does not affect pointer size

3. **Cast to qualified pointer type**
   - Test: cast `char *p` to `(const char *)` and dereference
   - Validates that const cast succeeds and dereference works

## Implementation order

1. Update parser lookahead conditions (sizeof and cast)
2. Update `parse_type_name()` to parse qualifier_specifier lists
3. Run tests against changes
4. Update `src/version.c` to stage 00820300
5. Update `docs/grammar.md` with new productions
6. Update `README.md` with feature summary
