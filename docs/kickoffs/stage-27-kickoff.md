# Stage 27 Kickoff: Declaration Specifier Refactor

## Spec Summary

Stage 27 refactors declaration specifier parsing to build declarations from a list of declaration specifiers rather than a fixed pattern of optional storage class plus one type specifier.

Currently, the parser expects: `[ <storage_class_specifier> ] <type_specifier>`

The new approach collects declaration specifiers as a sequence, allowing semantic validation to enforce rules like:
- No duplicate storage classes
- At most one type specifier
- At least one type specifier
- No storage classes at block scope

## Grammar Changes

**Old grammar:**
```ebnf
<declaration_specifiers> ::= [ <storage_class_specifier> ] <type_specifier>
```

**New grammar:**
```ebnf
<declaration_specifiers>   ::= <declaration_specifier> { <declaration_specifier> }
<declaration_specifier>    ::= <storage_class_specifier> | <type_specifier>
```

## Tokenizer Changes

None required.

## Parser Changes

- Add `parse_declaration_specifiers()` helper function to collect declaration specifiers as a sequence
- Validate that:
  - At least one specifier is present
  - At most one type specifier is present (detect `int int x` as invalid)
  - At most one storage class is present (detect `static extern int x` as invalid)
  - No storage classes appear at block scope (existing validation in `parse_statement`)
- Update `parse_external_declaration()` to use the new helper

## AST Changes

None required. The AST structure remains unchanged.

## Code Generation Changes

None required.

## Test Plan

**Invalid cases to add (4 new tests: 135-138):**
1. `static extern int x;` — duplicate storage classes
2. `extern static int x;` — duplicate storage classes (order reversed)
3. `int int x;` — duplicate type specifiers
4. `static char int x;` — duplicate type specifiers with storage class

**Existing tests:**
- Block scope static already covered by test_invalid_118
- All existing valid and invalid tests should continue to pass

## Implementation Order

1. Parser: add `parse_declaration_specifiers()` helper and update `parse_external_declaration()`
2. Tests: add 4 new invalid test cases
3. Docs: update `docs/grammar.md` with new declaration_specifiers production

## Decisions

- The new helper will return an error structure containing the parsed specifiers and any validation errors
- Validation is performed during parsing, not deferred to semantic analysis
- Block scope validation remains in `parse_statement()` as-is
- No changes to declarators, initializers, or function parameters at this stage
