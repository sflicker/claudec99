# Stage 20-02 Kickoff: Comma-Separated Init-Declarator Lists

## Summary of the Spec

Stage 20-02 extends the compiler to support comma-separated init-declarator lists in variable declarations. This enables declarations like:
- `int a, b;` (multiple variables)
- `int a=3, b=4;` (multiple variables with initializers)
- `int a = 5, b = a + 2;` (initializers can reference earlier variables)

The grammar change:
```ebnf
<declaration> ::= <type_specifier> <init_declarator_list> ";"

<init_declarator_list> ::= <init_declarator> { "," <init_declarator> }
```

This generalizes single declarations into a list pattern, with the type applying to all declarators in the list.

## Required Tokenizer Changes

**None.** TOKEN_COMMA already exists in the tokenizer.

## Required Parser Changes

Extend the declaration-parsing branch of `parse_statement()` in `src/parser.c`:

1. After parsing the first `init_declarator`, enter a loop checking for TOKEN_COMMA.
2. For each comma, consume it and parse another `init_declarator`.
3. Collect all parsed declarators.
4. Wrap multiple declarators in a new `AST_DECL_LIST` node; single declarators remain as-is for backward compatibility.

This maintains the existing structure for single declarations while transparently handling multiple declarators.

## Required AST Changes

1. Add `AST_DECL_LIST` to the AST node type enum in `include/ast.h`.
2. Define the node structure: `AST_DECL_LIST` holds a list of child declarator nodes.

## Required Code Generation Changes

1. Add `AST_DECL_LIST` handler in `codegen_statement()` in `src/codegen.c`.
2. The handler iterates over children and calls `codegen_statement()` on each declarator.
3. No changes needed to `compute_decl_bytes()`—it already handles children recursively.

## Required Pretty Printer Changes

Add an `AST_DECL_LIST` case in `src/ast_pretty_printer.c` to format the list of declarators for debugging output.

## Test Plan

Five tests derived from the spec:

1. **Multiple uninitialized variables**: `int a, b;` with separate assignments → expect 7
   - Verifies basic multi-declarator parsing and code generation.

2. **Multiple initialized variables**: `int a=3, b=4;` → expect 12
   - Verifies initializer lists are correctly applied.

3. **Forward reference in initializer**: `int a = 5, b = a + 2;` → expect 7
   - Verifies later declarators can reference earlier ones.

4. **Mixed pointer and non-pointer**: `int *p, q;` → expect 7
   - Verifies declarators with different type modifiers (pointers) in one list.

5. **Comma expression in initializer**: `int a = (1,2), b=3;` → expect 5
   - Verifies parenthesized comma expressions in initializers are not confused with list syntax.

## Known Ambiguities and Decisions

**Spec title typo**: The spec file header says "Stage-20-01" but the filename and content are correct for stage 20-02 (the prior stage, 20-01, was a declarator refactor). No ambiguity in scope.

**Arrays in declarator lists**: Array declarations (e.g., `int a[5], b;`) are not in scope for this stage. If attempted, the compiler will emit a clear error. This limitation can be addressed in a future stage.

**Backward compatibility**: Single `init_declarator` declarations continue to be parsed without wrapping in `AST_DECL_LIST`, preserving existing code paths and test compatibility.

## Implementation Order

1. Update `include/ast.h` with `AST_DECL_LIST` node type.
2. Modify `src/parser.c` to recognize and parse comma-separated declarators.
3. Add `AST_DECL_LIST` handler to `src/codegen.c`.
4. Add `AST_DECL_LIST` case to `src/ast_pretty_printer.c`.
5. Build and run test suite, verifying all five spec tests pass.
