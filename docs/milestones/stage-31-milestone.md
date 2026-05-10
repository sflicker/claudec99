# Milestone Summary

## Stage 31: Struct Member Access - Complete

stage-31 ships struct member access via `.` (dot) and `->` (arrow) operators, supporting both rvalue and lvalue contexts.

- Tokenizer: Added TOKEN_DOT and TOKEN_ARROW; lexer recognizes `.` and `->` in the appropriate branches.
- Grammar/Parser: Updated postfix_expression rule to include `"." <identifier>` and `"->" <identifier>` repetition; parser builds AST_MEMBER_ACCESS and AST_ARROW_ACCESS nodes; both node types allowed as lvalues in assignment.
- AST/Semantics: Added two new AST node types (AST_MEMBER_ACCESS, AST_ARROW_ACCESS); Type struct now includes fields array (StructField[]) and field_count to track member names and offsets.
- Codegen: Added find_struct_field helper; emit_member_addr and emit_arrow_addr compute field addresses; rvalue and assignment codegen for both node types; sizeof and type resolution updated.
- Tests: 3 new valid tests (struct dot, arrow, padding alignment), 3 new invalid tests (dot on non-struct, arrow on non-pointer, unknown field). Full suite 612/612 pass.
- Docs: grammar.md updated; milestone and transcript generated.
- Notes: Struct assignment, parameters, return values, and initializers remain out of scope. Grammar formatting corrected during implementation (`.` and `->` placed inside repetition group).
