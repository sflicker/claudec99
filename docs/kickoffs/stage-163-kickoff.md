# Stage 163 Kickoff — Non-Constant Initializer for Global (NULL Cast Bug)

## Spec Summary

Stage 163 fixes a parser validator bug that rejects `SDL_Window *window = NULL;`
when NULL is defined as `((void *)0)` (the standard C99 stddef.h definition).
The error message "non-constant initializer for global 'window'" is incorrect
because `(void *)0` is a null pointer constant per C99 and should be accepted.

The root cause: parser validator at `src/parser.c` ~line 4642 accepts
`AST_INT_LITERAL`, `AST_CHAR_LITERAL`, `AST_STRING_LITERAL`, `AST_VAR_REF`,
`AST_ADDR_OF`, and `AST_COMPOUND_LITERAL` for pointer global initializers,
but rejects `AST_CAST`. When NULL expands to `((void *)0)`, the parser produces
an `AST_CAST` node with an `AST_INT_LITERAL("0")` child, triggering the error.
GCC accepts this correctly.

## Stage Values

- STAGE_LABEL: stage-163
- STAGE_DISPLAY: Stage 163
- VERSION_STAGE: 01630000

## What Changes vs Stage 162

Stage 162 added a zlib integration test. Stage 163 is a compiler bug fix with
parser and codegen changes needed to handle null pointer constants in global
pointer initializers.

## Required Tokenizer Changes

None. The NULL macro expands correctly; the issue is in the parser validator.

## Required Parser Changes

- `src/parser.c` ~line 4642: Extend the pointer global initializer validator
  to accept `AST_CAST` nodes where the operand is `AST_INT_LITERAL(0)`. This
  recognizes null pointer constants like `(void *)0`, `(int *)0`, etc.

## Required AST Changes

None. No AST structure changes needed.

## Required Code Generation Changes

- `src/codegen.c`: Extend `codegen_add_global()` to handle `AST_CAST` nodes
  by extracting the null pointer constant and emitting the appropriate
  initialization. Cast nodes wrapping integer 0 should be treated as
  null pointer constants and registered as global pointers.

## Test Plan

1. Create `test/integration/` test case for `SDL_Window *window = NULL;`
   with `#include <stddef.h>` to ensure NULL macro expands to `((void *)0)`
2. Verify the test compiles without error
3. Run full test suite to confirm no regression
4. Verify self-host build succeeds (C0→C1→C2)
5. Update version, docs, and generate milestone/transcript artifacts

## Implementation Order

1. Extend parser validator to accept `AST_CAST` with integer-0 operand
2. Extend codegen to handle cast null pointer constants
3. Update `src/version.c` VERSION_STAGE to "01630000"
4. Create integration test
5. Run tests and self-host
6. Update docs and generate artifacts
