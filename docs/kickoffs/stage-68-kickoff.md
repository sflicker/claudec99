# Stage 68 Kickoff: Support More Than 6 Arguments in Function Calls

## Spec Summary

Stage 68 removes the hard-coded 6-argument limit for function calls and function definitions. The compiler will follow the System V AMD64 ABI to pass arguments 7+ on the stack (right-to-left order), while maintaining the existing 6-register calling convention (rdi/rsi/rdx/rcx/r8/r9).

## Key Requirements

**Argument Passing Strategy**

- Arguments 1-6: passed in registers (rdi, rsi, rdx, rcx, r8, r9)
- Arguments 7+: passed on stack, right-to-left (arg8 pushed before arg7)
- Stack arguments appear at [rbp+16], [rbp+24], [rbp+32], ... in the callee frame

**Stack Alignment**

- Before call: rsp must be 16-byte aligned
- If odd number of stack args: add padding word to maintain alignment
- Example: 7 args total (1 stack arg) requires push padding + push arg7 + call

**Supported Argument Types**

- Integer types (char, short, int, long, long long, signed/unsigned variants)
- _Bool
- Pointers (void*, function pointers, struct pointers)
- String literals
- Enum constants

## Derived Changes

**Parser/Semantic Analysis (src/parser.c)**

- Remove hard-coded child_count > 6 limit on function call arguments
- Remove hard-coded num_params > 6 limit on function definitions
- All other validation rules remain unchanged

**Code Generation (src/codegen.c)**

- Update AST_FUNCTION_CALL handling for N > 6 arguments
- Update AST_INDIRECT_CALL handling for N > 6 arguments
- Implement callee-side prologue to load stack args from positive rbp offsets
- Add padding logic when stack arg count is odd
- Implement right-to-left stack push order for arguments 7+

**AST/Type System**

- No changes required to AST structure

**Tokenizer**

- No changes required

## Known Issues from Spec

1. **Line 221 typo**: `unsigned int 7` should be `unsigned int f` (using `f` in implementation)
2. **Line 1 title**: Should say "function call" instead of "method call"
3. **Line 110**: Missing closing bold marker on "**Callee side parameter access*" (treat as closed)

## Implementation Order

1. Remove parser hard limits (child_count > 6, num_params > 6)
2. Update AST_FUNCTION_CALL codegen for 7+ args with stack alignment logic
3. Update AST_INDIRECT_CALL codegen for 7+ args
4. Add callee-side prologue to load stack args
5. Test with provided test cases

## Test Plan

**Unit tests (test/valid/)**

- `test_seven_arg_call__42.c`: Call with exactly 7 arguments
- `test_eight_arg_sum__42.c`: Call with 8 arguments, sum them
- `test_stack_arg_expr__42.c`: Stack arg containing an expression
- `test_stack_arg_pointer__42.c`: Stack arg as a pointer dereference
- `test_stack_arg_string__1.c`: Stack arg as string literal (strcmp test)
- `test_many_args_mixed_widths__42.c`: Mixed width integers in stack args

**Integration tests (test/integration/)**

- `test_printf_seven_args/`: Call printf with 7 format args

## Decisions

- Callee-side: load stack args directly from positive rbp offsets (simpler than copying to local slots)
- All validation beyond argument count remains unchanged
