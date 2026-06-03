# Stage 88 Kickoff: Hex and Octal Character Escapes

## Summary of the Spec

Stage 88 adds support for hex escape sequences (`\xNN`) and octal escape sequences (`\NNN`) in both character literals and string literals. These escape sequences are decoded to their byte equivalents during lexical analysis.

**Spec issues:**
- Line 7: backtick typo—should be `'\101'` (single quote, not backtick)
- Line 33: unclosed code block—missing closing triple-backtick

## Required Tokenizer Changes

The lexer (`src/lexer.c`) must be extended to recognize and decode escape sequences in character and string literals:

1. **Character literal escapes**: Extend the character literal parser to handle `\xNN` (2 hex digits) and `\NNN` (1-3 octal digits) in addition to existing single-character escapes.

2. **String literal escapes**: Extend the string literal parser to handle `\xNN` and `\NNN` escape sequences within string content.

3. **Decoding logic**: Both escape forms must decode to their numeric byte value:
   - Hex: interpret as hexadecimal digits
   - Octal: interpret as octal digits (1 to 3 digits allowed)

## Required Parser Changes

None. Escape sequences are fully resolved during tokenization; the parser receives the final decoded character/string value.

## Required AST Changes

None. Escape sequences do not introduce new syntax or AST node types.

## Required Code Generation Changes

None. Escape decoding is completed before code generation.

## Test Plan

1. **Character literal hex escape**: Test `'\x41'` equals `'A'` (verify hex decoding)
2. **Character literal octal escape**: Test `'\101'` equals `'A'` (verify octal decoding)
3. **String literal hex escape**: Test `"He\x6c\154o"` equals `"Hello"` (mixed hex and octal in strings)
4. **Combined character test**: Build string from mixed escape forms using `strcmp()`
5. **Combined string test**: Verify string literal with mixed escapes using `strcmp()`
6. **Remove invalid tests**: Delete `test/invalid/test_invalid_47__invalid_escape_sequence.c` and `test/invalid/test_invalid_59__invalid_escape_sequence.c` (hex escapes are now valid)

## Implementation Plan

1. Modify `src/lexer.c` to handle hex and octal escapes in character and string literals
2. Add integration test cases for the new escape forms
3. Update `src/version.c`: `00870000` → `00880000`
4. Update documentation: `docs/grammar.md` and `README.md`
