# stage-81 Kickoff

## Summary of the spec

Stage 81 is a header/constants maintenance release. Three main categories:

1. **Stub header updates**: Add two new library function declarations to test stub headers.
   - `int putchar(int)` to `test/include/stdio.h`
   - `void *calloc(size_t nmemb, size_t size)` to `test/include/stdlib.h`

2. **Compiler limits**: Raise four parser/codegen limits in `include/constants.h` from 64 to 256.
   - `PARSER_MAX_FUNCTIONS` (64 → 256)
   - `PARSER_MAX_GLOBALS` (64 → 256)
   - `MAX_GLOBALS` (64 → 256)
   - `MAX_LOCALS` (64 → 256)

3. **Codegen fix**: Remove the restriction that rejects `!` (logical-not) on pointer operands. In C99, `!` is a valid scalar-type operator; pointers are scalar and should be treated as 64-bit integers for the purpose of negation (`cmp rax, 0; sete al`).

## Required tokenizer changes

None. All operators and keywords are already tokenized.

## Required parser changes

None. Grammar already allows `!` on any primary expression.

## Required AST changes

None. Existing `AST_UNARY_OP` node type with `OP_LOGICAL_NOT` is sufficient.

## Required code generation changes

In `src/codegen.c`, the `codegen_expression` function rejects `!` on pointer operands with an error. This check must be removed, allowing pointers to be negated like 64-bit integers:

- Load the pointer value into a 64-bit register (rax)
- Compare rax against 0
- Emit `sete al` to set al to 1 if zero, 0 if nonzero
- Zero-extend al to rax
- Return the result

Update `version.c`: bump `VERSION_STAGE` from `00800000` to `00810000`.

## Test plan

Valid tests:
- Calloc: allocate 4 ints with calloc, verify zero-initialization, write one element, free, return 0
- Putchar: call `putchar('O')`, `putchar('K')`, `putchar('\n')`, expect output "OK\n"
- Putchar + calloc: allocate char buffer with calloc, write two elements, output via putchar, expect "OK\n"
- Logical not on NULL pointer: `if (!p)` where p is NULL should be true (evaluate to 1); `if (!p)` where p is non-NULL should be false (evaluate to 0)

Invalid tests:
- Remove `test_invalid_73__not_supported_on_pointer_operands.c` (was testing incorrect behavior)
- Remove `test_invalid_74__not_supported_on_pointer_operands.c` (same reason)

All existing tests must continue to pass.

## Implementation order

1. Headers: Add putchar to stdio.h, calloc to stdlib.h
2. Constants: Raise four limits in include/constants.h
3. Codegen: Remove the pointer rejection for `!` operator in src/codegen.c
4. Version: Bump VERSION_STAGE in src/version.c
5. Tests: Add valid test cases for calloc, putchar, combined usage, and null-pointer negation; remove two invalid tests
6. Docs: Update README.md (section about supported functions, test totals, stage reference); docs/grammar.md needs no changes

## Known ambiguities and decisions

**Pointer negation semantics**: The C99 standard treats `!` as a scalar-type operator that converts its operand to _Bool. For pointers, this conversion means: null (0) → 1 (true), non-null → 0 (false). This is identical to the behavior with integers and is handled by the same `cmp rax, 0; sete al` codegen pattern used for integers.

**Calloc semantics**: The spec test verifies that calloc returns a zero-initialized buffer via the check `if (p[0] != 0)`. This confirms that calloc allocates and zero-initializes, unlike malloc which does not zero-initialize.

**Limits justification**: The stage raises function, global, and local limits from 64 to 256 to support larger self-compile targets (the compiler uses more globals and functions than the previous limit allowed).
