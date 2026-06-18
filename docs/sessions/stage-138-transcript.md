# stage-138 Transcript: auto and register Storage-Class Specifiers

## Summary

Stage 138 resolves two defects reported in the stage spec: CC99-011 (`auto` storage class rejected) and CC99-012 (`register` storage class rejected). Both are valid C99 storage-class specifiers for block-scope object declarations.

`auto` declares a variable with automatic storage duration, identical to the default for block-scope objects. It is valid only at block scope; file-scope `auto` is a constraint violation. The specifier has no effect on code generation and is treated as `SC_NONE` after parsing.

`register` also has automatic storage duration but carries the additional semantic restriction that the address of the declared object cannot be taken (`&register_var` is a compile error per C99 §6.7.1p6). `register` is valid at block scope and as a function parameter qualifier. No optimization hints are emitted; it allocates identically to a plain automatic variable. File-scope `register` is also rejected.

## Changes Made

### Step 1: Tokenizer

- Added `TOKEN_AUTO` and `TOKEN_REGISTER` to the `TokenType` enum in `include/token.h`, placed after `TOKEN_INLINE`.
- Added keyword recognition in `src/lexer.c`: `"auto"` maps to `TOKEN_AUTO`, `"register"` maps to `TOKEN_REGISTER`.
- Added display names `"'auto'"` and `"'register'"` to `token_display_name`.

### Step 2: AST

- Added `SC_AUTO = 8` and `SC_REGISTER = 16` to the `StorageClass` enum in `include/ast.h`.
- Updated `ast_print_decl_qualifiers` in `src/ast_pretty_printer.c` to print `"auto "` and `"register "` for the new storage classes.

### Step 3: Parser

- In `parse_declaration_specifiers` (file-scope path): added rejection of `TOKEN_AUTO` and `TOKEN_REGISTER` with `"error: auto is not valid at file scope"` and `"error: register is not valid at file scope"`.
- In `parse_statement` (block-scope path): added handling before the `TOKEN_STATIC` block — `TOKEN_AUTO` consumes the keyword and delegates to `parse_statement` unchanged (SC_NONE behavior); `TOKEN_REGISTER` consumes the keyword, calls `parse_statement`, and stamps `SC_REGISTER` on all resulting `AST_DECLARATION` nodes.
- In `parse_parameter_declaration`: added `TOKEN_REGISTER` to the leading qualifier consume list alongside `TOKEN_CONST`, `TOKEN_VOLATILE`, and `TOKEN_RESTRICT`.

### Step 4: Code Generator

- Added `int is_register` field to `LocalVar` in `include/codegen.h`.
- Initialized `new_lv.is_register = 0` in `codegen_add_var`.
- Set `last_lv->is_register = (node->storage_class == SC_REGISTER)` after `codegen_add_var` in all four declaration paths in `codegen_statement`: struct/union, array, FP, and scalar.
- In `AST_ADDR_OF` codegen for local variable operands: added a check for `lv->is_register`; emits `"error: cannot take address of register variable 'NAME'"` if true.

### Step 5: Version

- Bumped `VERSION_STAGE` in `src/version.c` from `"13700000"` to `"13800000"`.

### Step 6: Tests

Added 5 tests:

| File | Suite | Expected |
|------|-------|----------|
| `test/valid/declarations/test_auto_storage_class__27.c` | valid | exit 27 |
| `test/valid/declarations/test_register_storage_class__27.c` | valid | exit 27 |
| `test/invalid/declarations/test_auto_file_scope__auto_is_not_valid_at_file_scope.c` | invalid | error: auto is not valid at file scope |
| `test/invalid/declarations/test_register_file_scope__register_is_not_valid_at_file_scope.c` | invalid | error: register is not valid at file scope |
| `test/invalid/declarations/test_register_address_of__cannot_take_address_of_register_variable.c` | invalid | error: cannot take address of register variable |

## Final Results

Build: clean (no errors, pre-existing warnings only).
All 1970 tests pass (1284 valid, 262 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). No regressions.
Self-host: C0→C1→C2 all 1970 tests pass with no source changes during bootstrap.

## Session Flow

1. Read spec, supporting files, and README.
2. Inspected `include/token.h`, `src/lexer.c`, `include/ast.h`, `include/codegen.h`, `src/parser.c`, `src/codegen.c` to understand current storage-class infrastructure.
3. Produced kickoff artifact via haiku-stage-artifact-writer.
4. Implemented tokenizer changes (TOKEN_AUTO, TOKEN_REGISTER).
5. Implemented AST changes (SC_AUTO, SC_REGISTER) and pretty-printer.
6. Implemented parser changes (file-scope rejection, block-scope handling, parameter qualifier).
7. Implemented codegen changes (is_register field, address-of check).
8. Bumped VERSION_STAGE to 13800000.
9. Built and verified; corrected error message format to match test-runner filename convention (no surrounding quotes).
10. Ran full test suite: 1970/1970.
11. Ran self-host bootstrap: C0→C1→C2 all passing.
12. Committed source changes (captured in self-host checkpoint commits), then committed tests, kickoff, and self-compilation report.
13. Generated milestone, transcript, grammar, checklist, and README updates.
