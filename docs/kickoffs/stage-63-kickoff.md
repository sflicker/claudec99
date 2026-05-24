# Stage 63 Kickoff

## Spec Summary

Stage 63 adds support for the C99 `_Bool` type, a new stdbool.h controlled header, and extends the existing limits.h controlled header.

### Key Features

- **_Bool keyword**: New token (TOKEN_BOOL) and type (TYPE_BOOL) representing C99's boolean type
- **Size and alignment**: 1 byte each, behaves as unsigned
- **Value normalization**: Any nonzero value assigned to _Bool becomes 1; zero stays 0
- **Restrictions**: `signed _Bool`, `unsigned _Bool`, `short _Bool`, and `long _Bool` are invalid
- **Integer promotion**: _Bool promotes to int (same as char/short) in expressions
- **stdbool.h macros**: `bool` = `_Bool`, `true` = 1, `false` = 0
- **limits.h extension**: Add `UINT_MAX` and `ULONG_MAX` defines

## Required Changes by Subsystem

### Tokenizer

- `include/token.h`: Add TOKEN_BOOL to TokenType enum
- `src/lexer.c`: Recognize `_Bool` keyword, add to token_display_name

### Type System

- `include/type.h`: Add TYPE_BOOL to TypeKind enum, add type_bool() declaration
- `src/type.c`: Add type_bool() singleton (size=1, align=1, is_signed=0), update type_kind_name and type_is_integer

### Parser

- `src/parser.c`:
  - Add TOKEN_BOOL in parse_type_specifier
  - Reject `signed _Bool`, `unsigned _Bool`, `short _Bool`, `long _Bool`
  - Add TOKEN_BOOL to all declaration-start guards (sizeof, cast, local decl, external decl)

### Code Generation

- `src/codegen.c`:
  - Add TYPE_BOOL to type_kind_bytes, data_init_directive, sizeof type switch
  - Add emit_bool_normalize helper function
  - Apply normalization on assignment/init to _Bool locals/globals

### Test Infrastructure

- `test/include/stdbool.h`: New controlled header with bool, true, false macros
- `test/include/limits.h`: Add UINT_MAX and ULONG_MAX defines
- `docs/grammar.md`: Update type_specifier rule to include "_Bool"

## Implementation Order

1. **Token support** (token.h + lexer.c)
2. **Type system** (type.h + type.c)
3. **Parser** (parser.c)
4. **Code generation** (codegen.c)
5. **Test files and headers** (stdbool.h, limits.h updates)
6. **Grammar documentation** (grammar.md)

## Test Coverage

The spec includes six test cases covering:

1. _Bool assignment with nonzero value (0 → 0, nonzero → 1)
2. _Bool with addition result (a + b collapse to 0 or 1 after assignment)
3. _Bool promotion in expressions (a is promoted to int for operations)
4. _Bool with stdbool.h macros (bool, true, false)
5. UINT_MAX define presence in limits.h
6. ULONG_MAX define presence in limits.h

## Spec Issues Noted

- Line 31: "integer likecan" is a typo (should be "integer-like")
- Line 56: stray "what" word
- Lines 49-54: missing closing ``` for code block
- stdbool.h listing missing `#endif`
- Line 169: `UINIT_MAX` is a typo for `UINT_MAX` (will use correct `UINT_MAX` in tests)

## Key Decisions

- Value normalization is applied on assignment/initialization to _Bool, not during expression evaluation
- _Bool is treated as integer-like for promotion and expression purposes
- The control flow: assign → normalize → store (not during arithmetic)
