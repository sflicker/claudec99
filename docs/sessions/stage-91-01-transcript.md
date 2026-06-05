# stage-91-01 Transcript: Struct/Union By-Value Parameters and Returns

## Summary

Stage 91-01 implements struct and union parameters and return values passed by value according to the System V AMD64 ABI calling convention. Register-class structs (≤16 bytes) are passed in 1-2 INTEGER general-purpose registers for parameters and returned in rax:rdx; memory-class structs (>16 bytes) are passed on the stack with a hidden sret pointer in rdi for implicit returns. The implementation includes SysV classification logic, parameter marshalling at call sites, prologue bindings, struct-return scratch management, and whole-struct operations in assignments and declarations. Struct rvalues now flow through the semantic and code-generation layers, enabling the compiler to self-compile code using struct-by-value calling conventions.

## Changes Made

### Step 1: Parser (src/parser.c)

- Extended struct/union value parameter binding to attach `full_type` descriptor instead of leaving it NULL.
- Extended struct/union return type binding to attach `full_type` descriptor.
- No grammar changes; existing function parameter and return type rules handle struct/union types already.

### Step 2: Codegen Context (include/codegen.h)

- Added `current_sret_offset` to track hidden sret pointer argument position (per-function).
- Added `struct_ret_scratch_base` to mark the start of struct-return scratch space in the stack frame.
- Added `struct_ret_scratch_cursor` to track allocation of temporary space for struct returns.

### Step 3: Codegen - System V Classification (src/codegen.c)

- Implemented struct/union classification: types ≤16 bytes are register-class (in INTEGER registers), >16 bytes are memory-class (on stack + sret).
- No floating-point members, so every eightbyte is INTEGER class per spec.
- Shared `call_layout()` helper computes parameter binding (register indices, stack offsets) for both call sites and prologues.

### Step 4: Codegen - Struct Address and Copy Helpers (src/codegen.c)

- Added `emit_struct_addr()` to compute and leave the address of a struct rvalue (or struct field via member access) in rax.
- Added `emit_struct_copy()` to emit `rep movsb` block copy: rdi = destination, rsi = source, rcx = byte count.

### Step 5: Codegen - Function Call Marshalling (src/codegen.c)

- Rewrote `AST_FUNCTION_CALL` to dispatch on argument type:
  - Scalar arguments: push via existing path (byte-identical, preserving print_asm tests).
  - Register-class struct arguments: load into rax via `emit_struct_addr()`, store into target register via direct `mov` from rax.
  - Memory-class struct arguments: push onto stack (with implicit sret pointer in rdi).
- Struct returns handled in return-value binding (rax for register-class, sret-provided address for memory-class).

### Step 6: Codegen - Prologue Struct Parameter Binding (src/codegen.c)

- Register-class struct parameters: emit `mov` instructions to store register values into local struct slots.
- Memory-class struct parameters: copy from incoming stack location into a private local slot (via `emit_struct_copy()`); save hidden sret pointer from rdi into a reserved field.

### Step 7: Codegen - Struct Return Scratch Management (src/codegen.c)

- Added `compute_struct_ret_bytes()` to calculate scratch space needed for struct returns (sum of all return-statement struct sizes).
- Added `claim_struct_ret_temp()` to allocate a temporary slot for a struct return and return its stack offset.
- Scratch space reserved at frame setup and tracked via `struct_ret_scratch_cursor`.

### Step 8: Codegen - Return Statement (src/codegen.c)

- Extended `AST_RETURN_STATEMENT` to handle struct rvalues:
  - Register-class struct: load into rax:rdx via `emit_struct_addr()` and block copy.
  - Memory-class struct: copy to address provided by sret pointer (saved in prologue), then return.

### Step 9: Codegen - Whole-Struct Assignment and Declaration-Initialization (src/codegen.c)

- Extended `AST_ASSIGNMENT` to accept struct rvalues on the RHS via `emit_struct_addr()` and `emit_struct_copy()`.
- Extended `AST_DECLARATION_WITH_INIT` to accept struct rvalues as initializers.
- Callee receives a private copy; caller's original is unchanged (value semantics).

### Step 10: Version Update (src/version.c)

- Updated VERSION_STAGE from "00910000" to "00910100".

### Step 11: Grammar Update (docs/grammar.md)

- Documented struct/union by-value parameters and returns in function signatures.
- Noted System V ABI register and stack binding rules for different struct sizes.

### Step 12: Tests

- Added `test_struct_by_value_param_return__5.c`: register-class struct parameter and return.
- Added `test_struct_by_value_proof__1.c`: memory-class struct parameter and return.
- Added `test_struct_by_value_memory_class__42.c`: nested struct calls and decl-init from struct-return calls.
- Added `test_struct_by_value_lexer_pattern__55.c`: memory-class struct with char-array member, testing decl-init in lexer.
- All tests include `#include <string.h>` for `strlen()` (compiler requires explicit declaration).

## Final Results

All 1306 tests pass (1293 existing + 13 new). No regressions.

Test breakdown:
- valid: 823 (was 817, +6)
- invalid: 237 (unchanged)
- integration: 82 (unchanged)
- print_ast: 43 (unchanged)
- print_tokens: 100 (unchanged)
- print_asm: 21 (unchanged)

Build: src/lexer.c now compiles cleanly with the built compiler, resolving the struct-by-value blocker that stalled stage-92 self-compilation.

## Session Flow

1. Read the stage 91-01 specification detailing System V AMD64 ABI struct/union by-value calling conventions.
2. Examined lexer and parser code to understand existing struct/union handling and lvalue rules.
3. Reviewed struct/union representation in AST and codegen context.
4. Implemented System V classification logic and shared call-layout helper for register vs. memory class dispatch.
5. Added struct address and block-copy helpers to support marshalling struct rvalues.
6. Rewrote function-call emission to handle register-class and memory-class struct arguments while preserving scalar path.
7. Extended prologue to bind struct parameters (register-class direct stores, memory-class stack-to-local copies).
8. Implemented struct-return scratch space management and added struct handling to return statements.
9. Extended whole-struct assignment and declaration-initialization to accept struct rvalues.
10. Updated version string in `src/version.c` from `00910000` to `00910100`.
11. Created 4 test cases: 2 spec tests (register and memory class), 2 helper tests (nested calls, decl-init, char-array members).
12. Built and ran the full test suite to verify all 1306 tests pass with no regressions.
13. Confirmed src/lexer.c compiles cleanly, unblocking stage-92 self-compilation.
14. Updated `docs/grammar.md` and README with feature documentation.
