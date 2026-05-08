# Milestone Summary

## stage 25-03: Indirect Function Calls Through Function Pointers - Complete

stage-25-03 ships support for indirect function calls through function pointers in two forms: `fp(args)` and explicit dereference `(*fp)(args)`.

- Tokenizer: No changes required; function call syntax already supported.
- Grammar/Parser: Updated `<postfix_expression>` to include call suffix; `parse_primary` and `parse_postfix` now recognize and construct `AST_INDIRECT_CALL` nodes for identifier-based calls and explicit dereference forms respectively; `parse_statement` now supports optional initializers on function-pointer declarations.
- AST/Semantics: Added `AST_INDIRECT_CALL` node type; `AST_DEREF` optimized to recognize function-pointer dereference as an identity operation (no memory load).
- Codegen: `codegen_expression` validates callee is function-pointer type, checks argument count and types, evaluates arguments with proper SysV AMD64 register allocation, and emits `call r10` indirect call instruction; related helper functions (`sizeof_type_of_expr`, `expr_result_type`) extended to handle indirect calls.
- Tests: 9 tests added (5 valid, 4 invalid) and 1 obsolete test removed; breakdown: 427 valid, 132 invalid, 66 print-AST, 46 print-tokens, 21 print-asm. Full suite 699/699 pass.
- Docs: Updated `docs/grammar.md` restriction note (replaced blanket "not supported" with specific limitation on complex expressions); updated `README.md` feature summary and test totals.
- Notes: Only direct-variable and explicit-dereference forms are supported; calling through arbitrary expressions (e.g., `get_fp()(args)`) is deferred to a future stage.
