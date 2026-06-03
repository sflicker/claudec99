# Stage 89 Kickoff: Adjacent String Literal Concatenation

## Summary of the Spec

Stage 89 adds support for adjacent string literal concatenation. Two or more adjacent string literal tokens are automatically concatenated into a single string literal. For example, `"hello " "world"` is equivalent to `"hello world"`. This is standard C behavior per the C99 standard.

**Grammar update**: `<string_literal> ::= TOKEN_STRING_LITERAL { TOKEN_STRING_LITERAL }`

**Spec issues:**
- Lines 41–49: unclosed code block for test 4 (missing closing triple-backtick before line 51)
- Test 2 (line 29) uses `strlen()` without `#include <string.h>` in the spec code block

## Required Tokenizer Changes

None. The lexer already tokenizes each quoted string separately as `TOKEN_STRING_LITERAL`. No changes to tokenization behavior are needed.

## Required Parser Changes

Modify `src/parser.c` in the `parse_primary_expression()` function (around line 1335) to handle adjacent string literals:

1. After consuming the first `TOKEN_STRING_LITERAL` and creating the initial AST node, add a while-loop to check for additional adjacent `TOKEN_STRING_LITERAL` tokens.
2. For each additional token found, decode its bytes and `memcpy()` them into the same node's `value` buffer.
3. Accumulate the total `byte_length` across all concatenated strings.
4. Advance past each additional token before checking for the next one.

## Required AST Changes

None. Reuse the existing `AST_STRING_LITERAL` node type with its existing `value` buffer (size 1024) and `byte_length` field.

## Required Code Generation Changes

None. String literal code generation reads `node->value` and `node->byte_length`, which will already contain the concatenated result from the parser.

## Test Plan

**Valid tests** (5 cases):
1. `test_adjacent_string_literal_concat_basic__1.c` — `s = "hello " "world"; return s[6] == 'w';`
2. `test_adjacent_string_literal_strlen__1.c` — `return strlen("abc" "def") == 6;` (with `#include <string.h>`)
3. `test_adjacent_string_literal_hex_escape__42.c` — `s = "\x41" "B"; return s[0] + s[1] - 89;`
4. `test_adjacent_string_literal_multiline__1.c` — multiline adjacent strings with `strcmp()` (with `#include <string.h>`)
5. `test_adjacent_string_literal_global__1.c` — file-scope string declaration `char *msg = "hello " "world";`

**Invalid tests** (1 case):
1. `test_invalid_adjacent_string_plus.c` — `char * a = "hello" + "world";` (addition operator must be rejected; only whitespace/newline adjacency is allowed)

## Implementation Plan

1. Modify `src/parser.c` in `parse_primary_expression()` to accumulate adjacent string literals
2. Add five integration test cases for valid concatenation
3. Add one integration test for the invalid case (binary operator between strings)
4. Update `src/version.c`: `00880000` → `00890000`
5. Update documentation: `docs/grammar.md` and `README.md`

## Derived Stage Values

- `STAGE_LABEL`: stage-89
- `STAGE_DISPLAY`: Stage 89
- `VERSION_STAGE`: 00890000
