# Stage 21-01 Kickoff: Function Declarator Refactor

## Spec Summary

Stage 21-01 refactors function declarations and definitions to use the general `<declarator>` machinery introduced in stage 20, removing the distinct `<function_declarator>` production. This unifies how function and object declarations are parsed.

**Core change:** Function prototypes and definitions now parse using `<type_specifier> <declarator>` instead of `<type_specifier> <function_declarator>`, where the `<declarator>` must contain a function form (e.g., `identifier(...)`).

**Grammar transitions:**
- `<function> ::= <type_specifier> <function_declarator> ( <block_statement> | ";" )` → `<function> ::= <type_specifier> <declarator> ( <block_statement> | ";" )`
- `<direct_declarator>` gains function form: `<identifier> "(" [ <parameter_list> ] ")"`
- `<parameter_declaration> ::= <type_specifier> <parameter_declarator>` → `<parameter_declaration> ::= <type_specifier> <declarator>`
- Remove `<function_declarator>` and `<parameter_declarator>` productions

**Valid examples:**
```C
int f();
int f() { return 1; }
int add(int a, int b);
int *identity(int *p) { return p; }
```

**Invalid (must reject):**
```C
int f { return 1; }  // f is an object declarator, not function declarator
```

## Required Tokenizer Changes

**None.** No new tokens introduced.

## Required Parser Changes

**File: `src/parser.c`**

1. **Extend `ParsedDeclarator` struct** — Add field `int is_function` to track whether the declarator is a function form.

2. **Refactor `parse_declarator` logic** — Extend `parse_declarator` to detect function declarators:
   - After parsing `<declarator>` (pointers and identifier), check if next token is `(`
   - If yes: set `is_function = 1`, leave `(` unconsumed for caller
   - If no: set `is_function = 0`

3. **Refactor `parse_parameter_declaration`** — Replace manual pointer-star loop with call to `parse_declarator`:
   - Parse `<type_specifier>`
   - Call `parse_declarator` (which handles `*` and identifier)
   - Use resulting `ParsedDeclarator`

4. **Refactor `parse_function_decl`** — Update to use general declarator path:
   - Parse `<type_specifier>`
   - Call `parse_declarator`
   - Verify `d.is_function == 1`; error if not (e.g., "function declarator required")
   - Proceed with existing `(`, parameter parsing, `)` handling
   - Continue with `( <block_statement> | ";" )`

## Required AST Changes

**None.** No new node types. Function definitions and prototypes continue to use existing AST structures.

## Required Code Generation Changes

**None.** Generated output is unchanged; refactor is purely structural.

## Test Plan

1. **Regression:** All 630 existing tests must pass.

2. **New invalid test:** Add test that rejects function definition where declarator lacks `()`:
   ```C
   int f { return 1; }
   ```
   Expected: Parse error (e.g., "function declarator required").

3. **Coverage:** Verify existing tests cover:
   - Zero-argument functions
   - Typed parameter lists with multiple parameters
   - Functions returning integers
   - Functions returning pointers
   - Function calls
   - Prototypes and definitions

## Notes and Ambiguities

- **Editorial note in spec:** Semantic rule section contains copy-paste artifact — says "because `x` is an object declarator" but example uses `f`. Harmless; both refer to non-function declarators.

- **Out-of-scope items:** Function pointers, parenthesized declarators, array parameters, mixed object/function declarations, multiple function declarators in one declaration.

- **Implementation order:** Extend `ParsedDeclarator` → modify `parse_declarator` → refactor `parse_parameter_declaration` → refactor `parse_function_decl` → test.
