# Stage 38 Kickoff: void Type and void Pointer

## Spec Summary

Stage 38 adds `void` as a type specifier to the ClaudeC99 compiler, enabling:
- Functions with void return type (`void f(void) { ... }`)
- Bare `return;` statements in void functions (and implicit return by falling off end)
- `void` as sole parameter syntax (`int f(void)` = zero parameters)
- `void *` as a generic object pointer type
- Implicit conversion between `void *` and any object pointer type
- Rejection of `void` as a variable type (non-pointer)
- Rejection of dereferencing or pointer arithmetic on `void *`

## Required Tokenizer Changes

**File: `include/token.h`**
- Add `TOKEN_VOID` to the `TokenType` enum

**File: `src/lexer.c`**
- Add `"void"` keyword recognition with display name `"'void'"`

## Required Parser Changes

**File: `src/parser.c`**

1. `parse_type_specifier()`: Add `TOKEN_VOID` case returning `type_void()`

2. Is-type-specifier guard checks: Add `TOKEN_VOID` to all locations that test for type specifiers:
   - `parse_statement()` (for declarations)
   - `parse_declaration_specifiers()`
   - `parse_cast()` (cast expressions)
   - sizeof disambiguation logic

3. Parameter lists: Update `parse_external_declaration()` to recognize `void` as a sole parameter, meaning zero-parameter list. Do not treat it as a parameter declaration.

4. Return statements: Update `parse_statement()` to allow `return;` with no expression

5. Validation: Reject non-pointer `void` variable declarations at parse time

## Required AST Changes

No AST structure changes required. Return statements with zero children represent bare `return;`.

## Required Code Generation Changes

**File: `src/codegen.c`**

1. Type sizing: `type_kind_bytes()` - Add `TYPE_VOID` case returning 0

2. Function prologue: Set `cg->current_return_type = TYPE_VOID` for void functions

3. Function epilogue: Emit implicit epilogue after void function body (standard prologue/pop/ret)

4. Return handling:
   - Bare `return;` (AST_RETURN_STATEMENT with 0 children): emit epilogue
   - `return <expr>` in void function: emit error
   - Bare `return;` in non-void function: emit error

5. Void function result usage:
   - Reject void function call result in return statement
   - Reject void function call result in assignment
   - Expression validation: reject using void as value context

6. Void pointer handling:
   - Add `is_void_ptr()` helper function
   - Add `pointer_types_assignable()` for void* compatibility
   - Allow object pointer → void * assignment
   - Allow void * → object pointer assignment
   - Reject dereference of `void *`
   - Reject pointer arithmetic on `void *`
   - Reject subscript on `void *`

## Test Plan

**Valid Tests:**
- `test_void_fn_global__42.c` — void function sets global, main returns it
- `test_void_fn_ptr_mutation__37.c` — void function mutates via pointer parameter
- `test_void_fn_bare_return__12.c` — void function with explicit bare return
- `test_void_param_list__35.c` — `int f(void)` means no args
- `test_void_ptr_assign__42.c` — void * assignment round-trip with int *
- `test_void_ptr_struct__30.c` — void * with struct pointer (corrected from spec)

**Invalid Tests:**
- `test_void_var_decl__cannot_declare.c` — `void x;` at function scope
- `test_void_fn_return_value__void_function.c` — `return 1;` in void function
- `test_void_empty_return_nonvoid__empty_return.c` — bare `return;` in int function
- `test_void_ptr_deref__cannot_dereference.c` — `*void_ptr` dereference
- `test_void_ptr_arith__pointer_arithmetic.c` — `void_ptr + 1` arithmetic
- `test_void_fn_result_in_return__cannot_use.c` — using void function result in return
- `test_void_fn_result_in_assign__cannot.c` — using void function result in assignment

## Implementation Order

1. **Tokenizer** - Add TOKEN_VOID keyword
2. **Type System** - Add TYPE_VOID to type.h/type.c
3. **Parser** - Handle void in type specifiers, parameters, return statements
4. **Code Generation** - Type handling, function epilogue, void pointer restrictions
5. **Tests** - Create and validate test cases
6. **Documentation** - Update grammar.md and README.md

## Known Issues in Spec

1. Valid test 2 comment says `// expect 77` but code sets `*p = 37`. Should be `// expect 37`.
2. Valid test 6 has multiple bugs: `sturct` typo, `p.x` on pointer (should be `p->x`), `pp = &p` without semicolon, missing semicolon after struct body. Requires rewrite.
3. Invalid test 3 uses `int bad(int);` (declaration with semicolon) instead of `int bad(int) {` (definition).
4. Spec title references "void keyword pointer" but should be "void type and void pointer".
