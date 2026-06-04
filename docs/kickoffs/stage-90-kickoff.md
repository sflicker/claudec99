# Stage 90 Kickoff: Hexadecimal Integer Literals

## Summary of the Spec

Stage 90 adds support for hexadecimal integer literals with `0x` or `0X` prefix. Examples: `0x0`, `0x2A`, `0X2A`, `0xffffffff`. Hex literals support the same suffixes as decimal integer literals: `U`, `L`, `UL`, `LU`, `LL`, `ULL`, and mixed-case variants. Invalid forms — bare `0x`/`0X` with no following hex digits, or a non-hex character immediately after the prefix — must be rejected with a diagnostic error.

**Spec issues (non-blocking):**
- Title typo: "hexadecimal integer liters" should be "hexadecimal integer literals"
- Body spelling: "hexidecimal" used twice; should be "hexadecimal"
- Second valid test code fence missing closing triple-backtick before `## Invalid Tests`

## Required Tokenizer Changes

Modify `src/lexer.c` in the `isdigit(c)` integer-literal branch:

1. After entering the branch, check for `0x`/`0X` prefix: `c == '0'` and next char is `x` or `X`.
2. If present: consume both prefix chars into `token.value`, then consume hex digits via `isxdigit()`.
3. If no hex digit follows the prefix, emit `error: invalid hexadecimal integer literal` and `exit(1)`.
4. Store the full literal (e.g., `0x2A`) in `token.value` for display in error messages and `--print-tokens`.
5. Use `strtoul(token.value, &end, is_hex ? 16 : 10)` — `strtoul` with base 16 accepts the `0x` prefix.
6. All suffix parsing (`U`/`u`/`L`/`l`) and type determination code is shared unchanged.

## Required Parser Changes

None. Hex literals produce `TOKEN_INT_LITERAL` tokens identical to decimal literals; the parser does not inspect the radix.

## Required AST Changes

None. The `AST_INT_LITERAL` node stores the evaluated `long_value`, not the source radix.

## Required Code Generation Changes

None. The code generator uses `node->long_value` for integer literal emission.

## Test Plan

**Valid tests:**
1. `test_hex_literal_basic__42.c` — `return 0x2A;` (expect 42)
2. `test_hex_literal_uppercase_ul_suffix__1.c` — `0X2AUL == 42UL` (expect 1)
3. Additional coverage: `0x0`, uppercase `0X`, various suffixes (`U`, `UL`, `ULL`)

**Invalid tests:**
1. `test_invalid_149__hex_literal_no_digits.c` — `return 0x;`
2. `test_invalid_150__hex_literal_bad_digit.c` — `return 0xG1;`

## Implementation Plan

1. Modify `src/lexer.c` to detect and parse hex literals
2. Add valid tests to `test/valid/`
3. Add invalid tests to `test/invalid/`
4. Update `src/version.c`: `00890000` → `00900000`
5. Update `docs/grammar.md`
6. Update `README.md`

## Derived Stage Values

- `STAGE_LABEL`: stage-90
- `STAGE_DISPLAY`: Stage 90 - Hexadecimal Integer Literals
- `VERSION_STAGE`: 00900000
