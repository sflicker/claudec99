# Milestone Summary

## Stage 38: void Type and void Pointer - Complete

stage-38 ships `void` as a complete type with support for void functions, void parameters, void return types, and generic void pointers.

- Tokenizer: Added `TOKEN_VOID` keyword token with display name `'void'`.
- Grammar/Parser: Added `void` to type specifiers; made return expression optional (`return;`); added `void` parameter-list form (`int f(void)` = zero parameters); parser rejects void object declarations and `sizeof(void)`.
- AST/Semantics: Added `TYPE_VOID` to TypeKind enum; semantic checks reject void function call results in assignments and return statements; void pointers have implicit conversion to/from any non-function pointer type; dereferencing, arithmetic, and subscripting void pointers are rejected.
- Codegen: `type_kind_bytes(TYPE_VOID)` returns 0; void functions emit implicit epilogue on fall-off-end; bare `return;` in void functions generates epilogue without return value; `pointer_types_assignable()` handles void* ↔ object pointer compatibility.
- Tests: 15 tests added (6 valid, 9 invalid). Full suite 791/791 pass (493 valid, 165 invalid, 24 print-AST, 88 print-tokens, 21 print-asm).
- Docs: Updated grammar.md (three rule changes + void restrictions section); updated README.md (stage reference, feature bullets, test counts).
- Notes: Spec test 2 had typo in comment; spec test 6 had multiple issues (typo, wrong member access, missing semicolon). All corrected in implementation.
