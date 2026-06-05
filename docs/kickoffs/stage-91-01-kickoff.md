# Stage 91-01 Kickoff: Struct-by-Value Parameters and Returns

## Summary of the Spec

Stage 91-01 adds support for passing structs by value as function parameters and returning structs by value from functions, following the System V AMD64 ABI specification. When a struct is passed by value, the function receives a copy and any mutations are local to that function; the caller's original struct is unchanged. Likewise, when a struct is returned by value, a copy is returned to the caller.

The implementation handles two classification types per SysV:

- **Register-class structs** (≤16 bytes): Passed and returned using 1–2 INTEGER eightbyte GP registers (rax, rdx for return; rdi, rsi, rdx, rcx, r8, r9 for parameters). The 16-byte Token in the spec tests falls into this category.
- **Memory-class structs** (>16 bytes): Passed on the stack; returned via a hidden pointer in rdi (sret calling convention). The project's ~290-byte Token (in `include/token.h`) is memory-class and unblocks stage-92 self-compilation.

## Required Tokenizer Changes

None. String and character tokenization is unchanged.

## Required Parser Changes

Modify `src/parser.c`:

1. **`parse_parameter_declaration()`**: Attach `full_type` to struct/union value parameters (currently left `NULL`). This ensures the full type information is available during code generation for struct parameters.

2. **Function-declaration parsing**: Set `func->full_type` for struct/union return types (currently only set for pointer returns). This signals to code generation that the return type is a struct.

## Required AST Changes

None. Reuse existing node types:
- `AST_PARAM` for parameters
- `AST_FUNCTION_DECL` for function declarations
- `AST_RETURN_STATEMENT` for returns
- `AST_FUNCTION_CALL` for function calls

## Required Code Generation Changes

Modify `src/codegen.c` to implement struct parameter passing and return value handling:

1. **SysV classification helper**: Add a shared function to classify a struct type (register-class ≤16 bytes vs. memory-class >16 bytes) and compute its eightbyte layout (number of INTEGER eightbytes for register-class; MEMORY flag for memory-class). Assign GP registers in order: rdi, rsi, rdx, rcx, r8, r9. Reserve rdi for the hidden sret pointer when the return type is memory-class.

2. **Block-copy helper**: Add a function to emit code for copying N bytes of a struct from source to destination (using `memcpy`-like inline moves or rep-movsq loops).

3. **Function call marshalling**: Rewrite `AST_FUNCTION_CALL` argument code generation to:
   - Handle struct arguments: copy register-class structs into designated registers; copy memory-class structs onto the stack
   - Pass the hidden sret pointer in rdi when the return type is memory-class
   - Materialize struct return values into a frame scratch temporary and leave the struct address in rax

4. **Stack frame management**: Reserve frame scratch space for struct-return temporaries in stack-size computation.

5. **Function prologue**: 
   - Size parameter slots for the full struct sizes
   - Store register-class struct parameters (1–2 registers) into their stack slots
   - Reference memory-class struct parameters in place via a negative rbp offset pointing to the incoming stack copy
   - Save the hidden sret pointer (rdi) for use in return statements

6. **Return statements**: 
   - Register-class: load eightbytes into rax (and rdx if 2-eightbyte) 
   - Memory-class: copy the struct into [saved sret pointer]; set rax = saved sret pointer

7. **Struct assignments and initialization**: Extend whole-struct assignment and struct declaration-initialization to accept a struct-returning function call (or general struct rvalue) as the source.

8. **Version update**: `src/version.c` → `VERSION_STAGE "00910100"`

## Test Plan

**Valid tests** (2 spec tests + coverage):

1. `test_struct_by_value_token_param_return__5.c` — Pass and return a 16-byte register-class Token by value; verify by-value semantics (caller's original is unchanged after the callee mutates its copy). Expected exit code: 5.

2. `test_struct_by_value_token_nested__1.c` — Nested struct call with Token by value; verify chained calls preserve by-value semantics. Expected exit code: 1.

3. **Coverage test (memory-class)**: Pass and return a >16-byte memory-class struct by value; verify sret mechanism and memory layout.

4. **Coverage test (register-class multiple fields)**: Multi-eightbyte register-class struct parameter and return.

All tests must pass via `./test/run_all_tests.sh`.

## Implementation Plan

1. Modify `src/parser.c` to attach `full_type` to struct/union parameters and return types
2. Add SysV classification and block-copy helpers in `src/codegen.c`
3. Rewrite function call argument marshalling and prologue code generation
4. Implement struct return value handling
5. Extend struct assignment and initialization for struct rvalues
6. Add integration test cases (2 spec + coverage)
7. Update `src/version.c`: `VERSION_STAGE "00910100"`
8. Update documentation: `docs/grammar.md` and `README.md`

## Derived Stage Values

- `STAGE_LABEL`: stage-91-01
- `STAGE_DISPLAY`: Stage 91-01
- `VERSION_STAGE`: 00910100
