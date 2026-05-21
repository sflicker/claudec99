# Stage 57-01 Kickoff: Function-Like Macro Stringification

## Spec Summary

Add support for the `#` operator (stringification) inside function-like macro replacement lists. When `#` precedes a macro parameter, the argument is converted into a string literal with quotes and backslashes properly escaped. Whitespace in the argument is normalized to single spaces.

Example:
```C
#define STR(x) #x
STR(hello)        // → "hello"
STR(1 + 2)        // → "1 + 2"
STR(NAME)         // → "NAME" (not expanded)
STR("HELLO")      // → "\"HELLO\""
```

## Tokenizer Changes

None required. The `#` operator is already tokenized in the preprocessor context.

## Parser Changes

None required. The preprocessor already parses function-like macro definitions.

## AST Changes

None required. No AST representation changes needed.

## Code Generation Changes

This is a preprocessor feature, not a code generation feature. All changes occur during macro expansion in `src/preprocessor.c`.

### Changes in preprocessor.c

1. **Add `stringify_arg()` function**
   - Takes raw argument text and normalizes whitespace
   - Escapes quotes and backslashes in the result
   - Returns a string literal (with surrounding quotes)

2. **Extend `substitute_args()` function**
   - Add parameters for raw argument text (`raw_args`, `raw_arg_lens`)
   - Detect `#` followed by parameter name in replacement text
   - Call `stringify_arg()` to produce string literal
   - Replace `#param` with the stringified result

3. **Add `#define`-time validation**
   - When parsing `#define`, validate `#` usage:
     - `#` is only valid in function-like macros (not object-like)
     - `#` must be followed by a parameter name
     - Skip over string/char literals to avoid false positives

4. **Capture raw arguments before macro expansion**
   - In `expand_macros_text()` and `preprocess_internal()`, capture raw argument tokens
   - Pass raw args separately to `substitute_args()` so arguments are not pre-expanded before stringification

## Test Plan

### Valid Tests (5)

1. **test_pp_str_simple** — `STR(coffee)` expands to `"coffee"`
2. **test_pp_str_expr** — `STR(1 + 2)` expands to `"1 + 2"`
3. **test_pp_str_whitespace_norm** — `STR(1     + 2)` expands to `"1 + 2"` (whitespace normalized)
4. **test_pp_str_no_expand** — `STR(NAME)` expands to `"NAME"` (argument not macro-expanded)
5. **test_pp_str_escape** — `STR("HELLO")` expands to `"\"HELLO\""`

### Invalid Tests (3)

1. **test_invalid_140** — `#define BAD(x) #y` (# not followed by a parameter)
2. **test_invalid_141** — `#define BAD(x) #` (bare # in replacement)
3. **test_invalid_142** — `#define BAD #x` (# in object-like macro)

## Implementation Order

1. Add `stringify_arg()` function
2. Add `#define`-time validation logic
3. Extend `substitute_args()` with stringification handling
4. Update call sites to capture and pass raw arguments
5. Verify existing tests still pass
6. Add new valid test cases
7. Add new invalid test cases

## Decisions and Ambiguities

- **Whitespace normalization**: Multiple consecutive spaces/tabs become a single space; leading/trailing whitespace is trimmed.
- **Escape handling**: Quotes and backslashes in stringified content are escaped as `\"` and `\\`.
- **Raw args capture**: Raw argument text is captured before any macro expansion occurs, ensuring the argument itself is not expanded before stringification.
- **Validation timing**: `#` validation happens at `#define` parse time, not lazily during expansion.
- **Literal skipping**: During `#define`-time validation, string and char literals are recognized and skipped to avoid matching `#` inside string content.
