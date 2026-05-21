# Stage 57-03 Kickoff: Variadic Function Calls and Declarations

## Summary of the Spec

This stage adds limited support for declaring and calling variadic functions like `printf`. 

**In-scope:**
- Declare variadic functions with `...` at the end of the parameter list: `int printf(const char *, ...);`
- Call variadic functions with at least the fixed parameter count; extra arguments allowed
- Type-check fixed parameters normally
- Support external libc variadic functions like printf

**Out of scope:**
- Defining variadic functions
- `va_list`, `va_start`, `va_arg`, `va_end`
- `<stdarg.h>`
- Floating-point variadic arguments

**Key ABI requirement:** For x86-64 System V, emit `xor eax, eax` before variadic function calls (sets AL=0 to indicate zero vector/FP registers used).

## Required Tokenizer Changes

1. **include/token.h**: Add `TOKEN_ELLIPSIS` token type to the enum
2. **src/lexer.c**: 
   - Register `...` to produce a single `TOKEN_ELLIPSIS` (currently tokenizes as three `TOKEN_DOT`)
   - Add display name for the new token

## Required Parser Changes

1. **include/parser.h**: Add `is_variadic` field to `FuncSig` struct to track variadic status
2. **src/parser.c**:
   - Modify `parse_parameter_list()` to detect `TOKEN_ELLIPSIS` and set `func->is_variadic`
   - Ensure `...` appears only at the end after at least one fixed parameter
   - Modify `parser_register_function()` to accept and store `is_variadic`
   - Modify call site validation: allow `child_count >= param_count` for variadic functions (instead of exact match)

## Required AST Changes

1. **include/ast.h**: Add `int is_variadic` field to `ASTNode` struct to propagate variadic flag through the AST

## Required Code Generation Changes

1. **src/codegen.c**: Before emitting `call` for variadic functions, emit `xor eax, eax` to set AL=0

## Test Plan

Create two valid tests:
1. **test/valid/test_printf_hello__0.c** + **.expected**
   - Declare `int printf(const char *, ...);`
   - Call `printf("hello\n");`
   - Expected output: `hello`

2. **test/valid/test_printf_int_arg__0.c** + **.expected**
   - Declare `int printf(const char *, ...);`
   - Call `printf("%d\n", 42);`
   - Expected output: `42`

## Grammar Update

Update **docs/grammar.md** `parameter_list` rule to document the optional trailing `...` token.

---

**Implementation approach:** Token → Lexer → Parser → AST → Codegen, then tests and grammar documentation.
