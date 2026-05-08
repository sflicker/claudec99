# stage 25-03 kickoff: Indirect function calls

## Summary

Allow calling through function pointer variables using two forms:
- `fp(args)` — direct function-pointer call  
- `(*fp)(args)` — explicit-dereference form

Both forms evaluate the function-pointer expression, validate that it has function-pointer type, validate argument count and types, and emit an indirect call instruction.

## Spec clarifications

The spec contained no blocking issues, but the following test cases required correction:

| Test case | Issue | Resolution |
|-----------|-------|-----------|
| Valid: "Function pointer parameter" | Test code was `apply(int, 6)` which doesn't match function signature | Corrected to `apply(inc, 6)` |
| Invalid: "Wrong argument count" | Test code was `add(1)` (direct call, not testing indirect path) | Corrected to `return fp(1)` to test indirect call with wrong arg count |
| Invalid: "Wrong argument type" | Test code was `read(3)` (direct call, not testing indirect path) | Corrected to `return fp(3)` to test indirect call with wrong arg type |

## Tokenizer changes

None required.

## Parser changes

**parse_primary (postfix position):**
- When an identifier is followed by `(`, check parser->funcs to determine if it's a known function
- If NOT in funcs, create an AST_INDIRECT_CALL node instead of emitting an error
- Pass arguments list through validation

**parse_postfix (call suffix):**
- Add handler for TOKEN_LPAREN to detect `(*expr)(args)` pattern
- When parsing the `(` after a dereference, create an AST_INDIRECT_CALL node
- Pass arguments list through validation

## AST changes

**include/ast.h:**
- Add `AST_INDIRECT_CALL` to the ASTNodeType enum
- Allocate a new call structure for indirect calls (similar to AST_CALL but with expression callee instead of function name)

## Code generation changes

**src/codegen.c - AST_DEREF handler:**
- When dereferencing a function pointer (base type is TYPE_FUNCTION), treat as identity operation
- Do not emit a memory load; function-pointer values are addresses already

**src/codegen.c - new AST_INDIRECT_CALL handler:**
- Evaluate callee expression (function pointer)
- Validate callee has function-pointer type
- Evaluate arguments in order
- Validate argument count matches function pointer signature
- Validate argument types are compatible with parameters
- Emit `call r10` (or appropriate register holding the function pointer address)
- Return type is the function's return type

**src/codegen.c - expr_result_type:**
- Add AST_INDIRECT_CALL case: return function's return type

**src/codegen.c - sizeof_type_of_expr:**
- Add AST_INDIRECT_CALL case: return function's return type

**src/ast_pretty_printer.c:**
- Add AST_INDIRECT_CALL case to print the node

## Test plan

**Valid tests (new):**
- `test_indirect_call_basic__5.c` — `fp(4)` where `fp` points to `inc` returns 5
- `test_indirect_call_explicit_deref__5.c` — `(*fp)(4)` returns 5
- `test_indirect_call_two_args__5.c` — two-arg function pointer `op(2,3)` returns 5
- `test_indirect_call_fp_param__7.c` — function pointer as parameter: `apply(inc, 6)` returns 7
- `test_indirect_call_reassign__20.c` — reassign `fp = dec; a=fp(10), b=dec(10), a+b=20`

**Invalid tests (new):**
- `test_invalid_130__*.c` — `int x = 3; x(1)` — non function pointer call
- `test_invalid_131__*.c` — `int *p = &x; p(1)` — pointer-to-int is not callable
- `test_invalid_132__*.c` — `fp(1)` when `fp` expects 2 args — wrong argument count
- `test_invalid_133__*.c` — `fp(3)` when `fp` expects `int*` — wrong argument type

## Implementation sequence

1. Add AST_INDIRECT_CALL to ASTNodeType enum
2. Implement parse_primary indirect call path
3. Implement parse_postfix call suffix for explicit dereference
4. Implement codegen AST_DEREF function pointer identity
5. Implement codegen AST_INDIRECT_CALL handler with full validation
6. Add expr_result_type and sizeof_type_of_expr cases
7. Add pretty printer case
8. Implement and validate tests
