# Stage 133 Kickoff — Default Argument Promotions In Function Calls

## Overview

Stage 133 implements C99 default argument promotion behavior for function calls
where the callee is declared without a prototype. Currently cc99 treats `int f();`
(empty parentheses, no `void`) as a zero-parameter prototype, causing "parameter
count mismatch" errors when the function is later defined with actual parameters.
In C99, `int f();` declares a function with no parameter type information and
requires default argument promotions to be applied.

## Spec Summary

Two regression tests define the required behavior:

1. **Variadic calls** (`test_default_argument_promotions__127.c`): cc99 already
   correctly applies default promotions to arguments after fixed parameters in
   variadic functions. Expected exit code: `127`. (Existing behavior, should not
   regress.)

2. **No-prototype calls** (`test_default_promotions_no_prototype__31.c`): A
   function declared as `int check();` (empty parens) must be callable with
   promoted arguments: char→int, short→int, unsigned short→int, float→double.
   Currently fails to compile with "parameter count mismatch (0 vs 5)". Expected
   exit code: `31`. (Currently failing; must be fixed.)

| Requirement | Current | Required |
|-------------|---------|----------|
| `int f();` representation | zero-parameter prototype | no-prototype declaration |
| Calls to no-prototype functions | parameter count validation | default promotions applied |
| `int f(void);` behavior | zero-parameter prototype | zero-parameter prototype (unchanged) |
| Variadic call behavior | correct promotions | correct promotions (no regression) |

## Required Changes

### 1. Tokenizer Changes

None.

### 2. Parser Changes

1. Add `int has_no_prototype;` field to the `FuncSig` struct in `include/parser.h`
   to track whether a function was declared with empty parentheses `()` versus
   explicit `(void)` or parameter list.

2. In `src/parser.c` function declaration parsing: detect when the parameter list
   is empty and not explicitly `void` (empty parens only). Set `is_no_prototype = 1`
   on the resulting `AST_FUNCTION_DECL` node.

3. Modify `parser_register_function()` in `src/parser.c` to accept an
   `is_no_prototype` parameter. When an existing function entry has
   `has_no_prototype = 1`, skip the parameter count mismatch check to allow any
   later definition.

4. At call sites (`src/codegen.c`), when validating argument counts: if the
   callee's signature has `has_no_prototype`, skip the count validation (any
   number of arguments is permitted and will be promoted).

### 3. AST Changes

Add `int is_no_prototype;` field to `ASTNode` in `include/ast.h`. Set this flag
on `AST_FUNCTION_DECL` nodes when the function is declared with `()` (empty
parens, no `void` keyword). This field is checked at call sites and during
redeclaration compatibility analysis.

### 4. Code Generation Changes (`src/codegen.c`)

The existing variadic argument promotion path (float→double) already handles most
promotions correctly. Extend the float→double promotion to also apply when
`callee->is_no_prototype`:

1. In the special (FP/struct) call path, Phase 1 (stack-allocated FP args):
   update the condition from `is_variadic` to `is_variadic || (callee &&
   callee->is_no_prototype)` when deciding whether to promote `float` to `double`.

2. In Phase 2 (register-allocated FP args): apply the same condition extension.

3. Integer narrow-type promotions (char/short→int) are automatically handled by
   existing load instruction semantics (movsx/movzx instructions sign/zero-extend
   values to register width).

### 5. Version Bump (`src/version.c`)

Change `VERSION_STAGE` from `"13200000"` to `"13300000"`.

## Test Plan

1. Add `test/valid/varargs/test_default_argument_promotions__127.c` — variadic
   test case covering char→int, short→int, unsigned short→int, float→double
   promotions. Should already pass (verifies no regression of existing behavior).
   Expected exit code: `127`.

2. Add `test/valid/functions/test_default_promotions_no_prototype__31.c` — call
   test case for no-prototype function with mixed argument types requiring
   promotion. Currently fails to compile; must compile and return `31` after
   implementation.

3. Run full test suite to verify no regressions.

4. Self-host cycle: `./build.sh --mode=self-host`.

## Implementation Order

1. `include/ast.h` — add `is_no_prototype` field to `ASTNode`
2. `include/parser.h` — add `has_no_prototype` field to `FuncSig`
3. `src/parser.c` — empty-parens detection, `parser_register_function` update,
   call site validation
4. `src/codegen.c` — extend float→double promotion path with no-prototype check
5. `src/version.c` — version bump to `13300000`
6. New test files (two regression tests)
7. Full test suite verification
8. Self-host bootstrap
9. Commit

## Notes & Decisions

- **C99 distinction**: `int f();` (no prototype) vs. `int f(void);` (zero-parameter
  prototype) are semantically distinct. This stage implements that distinction
  for the first time in cc99.

- **AMD64 ABI**: On SysV AMD64, `unsigned short` values fit in `int`, so
  `unsigned short` promotes to `int` (not to `unsigned int`).

- **Integer promotions**: Char and short promotions to int are handled by existing
  x86-64 load instruction semantics and don't require special code generation
  logic.

- **Float promotion scope**: The float→double promotion must apply to all arguments
  in no-prototype calls, not just variadic trailing arguments.

- **Compatibility**: A no-prototype declaration must accept a later prototype
  definition with any parameter count. Existing prototype mismatch diagnostics
  should be preserved for genuinely incompatible redeclarations.
