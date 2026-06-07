# Stage 95-09 Kickoff: AST Node Value Pointer Refactor

## Spec Summary

Stage 95-09 converts the `ASTNode.value` field from a fixed-size embedded character array (`char value[MAX_NAME_LEN]`) to a pointer into lexer-owned storage (`const char *value`). This eliminates the arbitrary 255-byte limit on AST node value strings and completes the elimination of fixed-capacity buffers from the compiler's core data structures.

**Key design principles:**
- All `astnode->value` pointers must originate from lexer storage (via `lexer_store_bytes`) or from another `ASTNode.value` that already does
- Static C string literals (for operator names like `"+"`, builtin names, empty-string sentinels) are acceptable since they have permanent storage
- AST nodes do not own the text; they reference it

**Affected node types:**
- `AST_STRING_LITERAL` — value is now direct pointer instead of copied buffer
- `AST_CHAR_LITERAL` — value is now direct pointer instead of copied buffer
- `AST_VAR_REF`, `AST_PARAM`, `AST_ASSIGNMENT`, and operators — named values via pointer
- `AST_BUILTIN_VA_ARG` — type name via pointer

## Tokenizer Changes

None.

## Parser Changes

- `include/parser.h`:
  - Change `ParsedDeclarator.name` from `char name[256]` to `const char *name`
  - Add `#include "strbuf.h"` for concatenated string literal handling

- `src/parser.c`:
  - String literal (single): Replace `memcpy(node->value, token.value, token.value_len)` with `node->value = token.value`
  - String literal (concatenated): Use `StrBuf` scratch buffer + `lexer_store_bytes` to build combined value
  - Remove `MAX_NAME_LEN` length checks on string literals (no longer bounded)
  - Char literal: Replace `node->value[0] = token.value[0]; node->value[1] = '\0'` with `node->value = token.value`
  - `__builtin_va_arg` type name: Replace `snprintf(node->value, ...)` with `node->value = type_kind_name(...)`
  - Enum constant int literal: Store buffer via `lexer_store_bytes` before assigning to node
  - Local and file-scope char array string initializers: Replace `memcpy + null-term` with `node->value = str_tok.value`
  - Unnamed param null checks: Replace `value[0] == '\0'` with `!value || value[0] == '\0'` (3 sites)
  - ParsedDeclarator assignments: Replace `strncpy(d.name, name.value, ...)` with `d.name = name.value` (2 sites)

## AST Changes

- `include/ast.h`:
  - Change `char value[MAX_NAME_LEN]` field to `const char *value` in `struct ASTNode`

- `src/ast.c`:
  - Update `ast_new` function: Replace `strncpy(node->value, value, sizeof(node->value) - 1)` with simple assignment `node->value = value`

## Code Generation Changes

- `src/codegen.c`:
  - Add NULL guards on 5 sites where `value[0]` is dereferenced for nodes that can have NULL value:
    - `AST_ASSIGNMENT`: 1 site
    - Function parameters: 4 sites

- `src/ast_pretty_printer.c`:
  - Add NULL guard on 1 site where `value[0]` is dereferenced for `AST_ASSIGNMENT`

## Test Plan

1. **Regression test** — Run full existing test suite (1478 tests) to confirm no regressions

2. **Long string literal test** — Verify a string literal longer than 255 characters is handled correctly (previously would be silently truncated by `ASTNode.value` capacity):
   ```c
   "very long string literal exceeding the old 255-byte MAX_NAME_LEN limit that was previously silently truncated"
   ```

3. **AST output validation** — Verify `--print-ast` output is still correct for string and char literal nodes

4. **Self-host validation** — Verify bootstrap pipeline (C0 → C1 → C2) succeeds and produces consistent binaries

## Implementation Order

1. Expose `lexer_store_bytes` in `include/lexer.h` by removing `static` from `src/lexer.c`
2. Update `include/ast.h` field declaration; update `src/ast.c` implementation
3. Update `include/parser.h` and `src/parser.c`:
   - ParsedDeclarator.name field change
   - String literal handling (single and concatenated)
   - Char literal handling
   - Builtin type name handling
   - Enum constant handling
   - Unnamed param null checks
4. Update `src/codegen.c` NULL guards on value dereferences
5. Update `src/ast_pretty_printer.c` NULL guard on value dereference
6. Update `src/version.c` to `VERSION_STAGE = "00950900"`
7. Run full test suite after each major section
8. Bootstrap validation (C0 → C1 → C2)
9. Update `docs/fixed-capacity-inventory.md` to document completion of AST node value refactor

## Spec Issues Noted

1. Spec title contains typo: "statis" should be "static"

## Ambiguities & Decisions

**Decision: ParsedDeclarator.name pointer safety**
- Changed to `const char *` to avoid dangling pointers when `d.name` is passed to `parser_node`
- Safe because `d.name` is always set from `token.value`, which is lexer-owned and stable throughout parsing

**Decision: Unnamed parameters and empty strings**
- Continue using `""` (static C string literal with permanent storage) rather than NULL
- Avoids breaking callers (`strcmp`, `codegen_add_var`) that expect a valid string, not NULL
- Static string literals have permanent storage lifetime, so storing pointer is safe

**Decision: Operator strings**
- Static C string literals like `"+"`, `"-"`, etc. are passed directly to `parser_node`
- These have permanent storage and are safe to store as `const char *`
- No need to copy or register with lexer

**Decision: String length preservation**
- `sizeof("literal")` and `strlen(child->value)` remain valid
- `lexer_store_bytes` always null-terminates stored bytes, enabling strlen compatibility

