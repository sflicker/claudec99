# Milestone Summary

## stage-85: Member-Array to Pointer Decay - Complete

stage-85 implements array-to-pointer decay for struct and union members: when an array-typed member is accessed in a value context (e.g., passed to a pointer parameter), the member address decays to a pointer to the element type. Also adds support for initializing char-array struct members from string literals in brace initializers.

- Tokenizer: No changes.
- Grammar/Parser: No changes (member access and struct initialization syntax already supported).
- AST/Semantics: codegen_expression (AST_MEMBER_ACCESS, AST_ARROW_ACCESS) now reports TYPE_POINTER with full_type=type_pointer(element) for array-typed fields; expr_result_type updated to match. emit_local_struct_init extended to initialize char-array fields from string-literal initializers via per-byte stores.
- Codegen: Member address computation (emit_member_addr, emit_arrow_addr) left in rax; for array fields, rax treated directly as the decayed pointer value with no load emitted. String-literal initializer bytes copied field-by-field.
- Tests: 2 new valid tests (test_member_array_decay_dot__42.c, test_member_array_decay_arrow__42.c); full suite 791 valid, 235 invalid, 73 integration, 43 print-AST, 100 print-tokens, 21 print-asm; 1263 total. All pass.
- Docs: stage-85-kickoff.md written during planning.
- Notes: Spec tests required char-array struct-member string-literal initialization, which the compiler previously rejected; now supported as a minimal per-byte copy matching existing local array string-init logic.
