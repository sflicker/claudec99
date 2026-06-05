# Milestone Summary

## Stage 91-01: Struct/Union By-Value Parameters and Returns - Complete

stage-91-01 implements struct and union parameters and return values passed by value according to the System V AMD64 ABI, enabling full struct/union value semantics in function calls.

- Tokenizer: No changes.
- Grammar/Parser: No grammar changes. Parser now attaches `full_type` to struct/union value parameters and struct/union return types (were previously NULL).
- AST/Semantics: Existing AST node types reused; semantic refinement adds type information to parameter and return bindings.
- Codegen: Implemented System V classification for struct/union types (register-class ≤16 bytes, memory-class >16 bytes). Added shared call-layout helper for both call sites and prologues. New helpers: `emit_struct_addr()` for field addresses, `emit_struct_copy()` for block copying via `rep movsb`, `compute_struct_ret_bytes()` and `claim_struct_ret_temp()` for struct-return scratch space. Rewrote `AST_FUNCTION_CALL` with struct-aware marshalling while preserving scalar push/pop behavior. Extended prologue to handle register-class struct binding (direct stores) and memory-class struct binding (copy from stack into private local, save hidden sret pointer). Added struct-return handling in `AST_RETURN_STATEMENT`. Extended whole-struct assignment and declaration-initialization to accept struct rvalues. Added `struct_ret_scratch_base`, `struct_ret_scratch_cursor`, `current_sret_offset` fields to codegen context.
- Tests: Four new valid tests added—spec tests for register-class and memory-class, plus helpers for nested calls and decl-init from calls. Full suite: 1306 passed (823 valid, 237 invalid, 82 integration, 43 print_ast, 100 print_tokens, 21 print_asm).
- Docs: Grammar and README updated to document struct/union by-value semantics.
- Notes: Tests include `#include <string.h>` for `strlen()` declarations (compiler requires explicit declarations). No inline struct literals yet; struct values must originate from variables, function returns, or explicit copies. Result unblocks stage-92 self-compilation.
