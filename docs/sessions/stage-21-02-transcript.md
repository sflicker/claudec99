# stage-21-02 Transcript: Separate Function Definitions from Declarations

## Summary

Stage 21-02 refactored the external declaration parser to separate function definitions (functions with a body) from function declarations (function prototypes). Previously, `parse_external_declaration` simply delegated to `parse_function_decl`, which treated all functions uniformly. The new implementation parses the common type_specifier + declarator prefix once, then dispatches based on the declarator kind (function vs. non-function) and the next token (`{`, `;`, or `=`). This enables cleaner semantics: function definitions must have a body, function declarations must not have an initializer, and non-function declarators cannot be followed by `{`. The AST node structure remains unchanged (both definitions and prototypes are AST_FUNCTION_DECL nodes, distinguished by the presence or absence of the body field).

## Changes Made

### Step 1: Parser Refactor

- Removed the `parse_function_decl` function from `src/parser.c`.
- Refactored `parse_external_declaration` to:
  - Parse type_specifier and declarator as a common prefix.
  - Determine the declarator kind via the `is_function_declarator` helper.
  - Dispatch on (declarator_kind, next_token) pairs:
    - Non-function + `{` → error "not a function declarator"
    - Non-function + `;` → AST_DECLARATION (file-scope object declaration)
    - Function + `=` → error "function declaration cannot have an initializer"
    - Function + `{` → AST_FUNCTION_DECL with body (definition)
    - Function + `;` → AST_FUNCTION_DECL without body (prototype)

### Step 2: Tests

- Added `test/valid/test_proto_identity_ptr__7.c`: function prototype with pointer return type followed by definition and use in main.
- Added `test/invalid/test_invalid_106__function_declaration_cannot_have_an_initializer.c`: rejected `int f() = 3;` with appropriate error message.

### Step 3: Grammar Documentation

- Updated `docs/grammar.md` to replace the monolithic `<function>` rule with two separate rules:
  - `<external_declaration>` now branches to `<function_definition>` or `<declaration>`.
  - `<function_definition>` is `<type_specifier> <declarator> <block_statement>`.
  - Restrictions clarified: function_definition must have a function declarator; file-scope declarations cannot have initializers (for functions).

### Step 4: README Update

- Updated README.md "What the compiler currently supports" section header from "Through stage 21-01" to "Through stage 21-02".
- Updated test count in the test suite table from 631 to 633.

## Final Results

All 633 tests pass (631 baseline + 2 new = 633 total). No regressions. Build successful.

## Session Flow

1. Read spec, templates, and current documentation.
2. Reviewed completed changes in src/parser.c, test files, and spec.
3. Generated milestone summary focusing on subsystems touched and test results.
4. Updated docs/grammar.md external_declaration and function_definition rules.
5. Updated README.md stage marker and test totals.
6. Verified all output files are in the correct locations with correct content.
