# Milestone Summary

## Stage 78: General Postfix Expression Chaining - Complete

stage-78 enables postfix operators (subscript, member access, function call, increment/decrement) to chain naturally on any postfix-expression base, not just variables.

- **Tokenizer**: No tokenizer changes required.
- **Grammar/Parser**: Parser `parse_postfix` function already correctly loops over postfix suffixes starting from a primary expression. Struct/union field parsing extended to wrap array-typed members with `type_array()` when `d.is_array && d.has_size` — this ensures array fields have kind `TYPE_ARRAY` instead of `TYPE_INT`.
- **AST/Semantics**: No new AST node types. Subscript base validation expanded to accept `AST_MEMBER_ACCESS` and `AST_ARROW_ACCESS` nodes in addition to `AST_VAR_REF`, `AST_DEREF`, and `AST_ARRAY_INDEX`.
- **Codegen**: `emit_array_index_addr` extended with two new cases for `AST_MEMBER_ACCESS` and `AST_ARROW_ACCESS` bases. For array-typed fields, the result register is already the base address; for pointer-typed fields, a dereference load (`mov rax, [rax]`) is inserted before indexing.
- **Tests**: Added 7 valid tests (arrow/dot + subscript chains, subscript + dot chains, multi-level chaining on arrays and pointers) and 4 invalid tests (subscript on non-array member, member/arrow after non-struct/non-pointer subscript). Full suite: 1230 passed (765 valid, 231 invalid, 71 integration, 43 print-AST, 99 print-tokens, 21 print-asm).
- **Docs**: Grammar unchanged — postfix-expression production correctly allowed chaining throughout. README updated to "through stage 78" and expanded Structs/Unions section to document array fields and chaining patterns like `p->tokens[i].kind`.
- **Notes**: Three spec test cases had missing closing braces (corrected in implementation). "Nested member subscript" test had bounds error (pos = 4 on array[4], changed to pos = 3). "Arrow after non-pointer subscript" invalid test had struct syntax error (changed `[` to `{`).
