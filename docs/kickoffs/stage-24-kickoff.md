# Stage 24 Kickoff: Parenthesized Declarators

## Spec Summary

Stage 24 adds parenthesized declarators as a grouping syntax in the declarator grammar. This allows declarators to be wrapped in parentheses for disambiguation, without introducing new semantics.

**Key examples:**
- `int (*p);` declares a pointer to int (equivalent to `int *p;`)
- `int (**pp);` declares a pointer to pointer to int (equivalent to `int **pp;`)
- Parentheses group existing pointer/object/array declarators

**Out of scope:**
- Function pointers: `int (*fp)(int);`
- Pointer-to-array types: `int (*p)[10];`
- Function returning pointer to function: `int (*f())(int);`

The stage treats parenthesized declarators as pure grouping syntax that resolves to already-supported types.

## Tokenizer Changes

**No changes required.** Parenthesized declarators use only existing tokens: `(`, `)`, `*`, and identifiers.

## Parser Changes

**In `src/parser.c`:**

1. Extend `parse_declarator()` to recognize parenthesized declarators
2. After parsing a `(`, check if the next token is `*` or an identifier; if so, parse the parenthesized declarator group:
   - Accept `{ "*" }` zero or more pointer specifiers
   - Accept an identifier
   - Optionally accept `[ [ <integer_literal> ] ]` array declarators
   - Consume closing `)`
3. After the closing `)`, reject further suffixes:
   - Reject `(` (would form function pointer)
   - Reject `[` (would form pointer-to-array)
4. Preserve existing behavior for non-parenthesized declarators

**Grammar change:**
```
<direct_declarator> ::= <identifier>
                     | <identifier> "[" [ <integer_literal> ] "]"
                     | <identifier> "(" [ <parameter_list> ] ")"
                     | "(" <declarator> ")"
```

Note: The correct form is `"(" <declarator> ")"` (no leading identifier); the spec has a grammar error showing `<identifier> "(" <declarator> ")"`.

## AST Changes

**No changes required.** Parenthesized declarators resolve to existing declarator AST nodes (pointers, arrays, identifiers).

## Code Generation Changes

**No changes required.** Parenthesized declarators produce the same code generation as their non-parenthesized equivalents.

## Test Plan

**Valid tests (6):**
1. Local grouped pointer (test 7) — `int (*p) = &x;` in block scope
2. Double pointer (test 9) — `int (**pp) = &p;` with nested grouping
3. File-scope grouped pointer (test 5) — `int (*p);` without initializer
4. Static grouped pointer (test 11) — `static int (*p);`
5. Parameter grouped pointer (test 13) — Function parameter `int (*p)`
6. Extern grouped pointer (test 17) — `extern int (*p);` with later definition

**Invalid tests (4):**
1. Function pointer declaration (test 119) — `int (*fp)(int);` should be rejected
2. Pointer-to-array (test 120) — `int (*p)[10];` should be rejected
3. Function pointer parameter (test 121) — Parameter `int (*fp)(int)` should be rejected
4. Function returning pointer to function (test 122) — `int (*f())(int);` should be rejected

## Spec Issues Identified

1. **Grammar error (line 22):** Spec shows `<identifier> "(" <declarator> ")"` but the correct parenthesized declarator form is `"(" <declarator> ")"` without a leading identifier. The spec mixes two separate cases.

2. **Double-pointer test (line 93):** Shows `return **p;` but should be `return **pp;` to match the variable declaration `int (**pp)`.

3. **File-scope test (line 104):** Missing semicolon — `return *p` should be `return *p;`.

4. **Deferred feature (line 49-55):** "Grouped pointer return declarations" is marked "may be deferred" — this stage treats function return types with parenthesized declarators as out of scope.

## Implementation Order

1. **Parser** — Extend `parse_declarator()` to detect and handle parenthesized groups; reject function pointer and pointer-to-array suffixes
2. **Valid tests** — Add test files for local, double pointer, file-scope, static, parameter, and extern cases
3. **Invalid tests** — Add test files for function pointer, pointer-to-array, function pointer parameter, and function returning function pointer
4. **Grammar doc** — Update `docs/grammar.md` with corrected `<direct_declarator>` rule
5. **Verification** — Run all tests; ensure invalid cases produce compile-time errors

## Key Decisions

- Parenthesized declarators are treated as syntax sugar that groups existing declarators; they do not introduce new semantics
- Function pointers and pointer-to-array types are explicitly rejected even if they appear parenthesized
- After a closing `)`, no further function or array suffixes are allowed
- The implementation preserves backward compatibility with non-parenthesized declarators
