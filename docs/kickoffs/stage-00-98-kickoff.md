# Stage 00-98 Kickoff - Benchmark Support

## Spec Summary

Stage 00-98 adds support for two benchmark programs to the valid test suite and fixes the issues that caused them to fail. The stage has three key components:

1. **U/u/UL/ul/LU/lu suffix support on integer literals**: The lexer currently only recognizes `L`/`l` suffixes on integer constants. The parser fails on `42U` with "expected ';', got identifier ('U')". Add full suffix parsing supporting any combination of U/u and L/l.

2. **Bug fix: unsigned function parameter handling**: Function parameters with unsigned types never had their `is_unsigned` flag propagated to the LocalVar entry. The codegen parameter registration loop calls `codegen_add_var()` but then never sets `cg->locals[cg->local_count - 1].is_unsigned`. This caused unsigned function parameters to behave as signed — most critically, right shifts on unsigned parameters used `sar` (arithmetic/signed) instead of `shr` (logical/unsigned).

3. **Benchmark test file naming**: The test file `test_bench_claudec99__0.c` needs to be renamed to `test_bench_claudec99__245.c` to reflect the correct exit code (245, verified against GCC).

## Tokenizer Changes

1. Update `include/token.h`: Add `int is_unsigned;` field to Token struct to track presence of U/u suffix
2. Update `src/lexer.c`:
   - Replace single `L`/`l` suffix check with a loop that consumes both U/u and L/l suffixes in any order
   - Set `token.is_unsigned = 1` when U/u suffix is present
   - Handle all valid combinations: U, u, L, l, UL, Ul, uL, ul, LU, Lu, lU, lu

## Parser Changes

1. Update `src/parser.c`:
   - For AST_INT_LITERAL nodes, propagate the `is_unsigned` flag from the token: `node->is_unsigned = token.is_unsigned`
   - No grammar changes; the suffix parsing is entirely in the lexer

## AST Changes

The `is_unsigned` field in ASTNode (added in stage 40) is already present and will be used for integer literal nodes.

## Code Generation Changes

1. Update `src/codegen.c`:
   - In the function parameter registration loop, after calling `codegen_add_var()`, add: `cg->locals[cg->local_count - 1].is_unsigned = node->children[i]->is_unsigned;`
   - This ensures unsigned function parameters are correctly marked in the LocalVar table
   - Add a comment noting that `is_unsigned` is preserved from the parser for consistency

## Test Plan

1. **Suffix parsing**: Verify lexer accepts all combinations (42U, 42u, 42L, 42l, 42UL, 42LU, 42ul, 42lu, etc.)
2. **Integer literal signedness**: Verify `AST_INT_LITERAL` nodes have correct `is_unsigned` flag
3. **Unsigned right shift on function parameters**: Verify benchmark code that right-shifts unsigned parameters generates `shr` not `sar`
4. **Benchmark execution**: Both `test_bench_claudec99_int__0.c` and `test_bench_claudec99__245.c` should execute and return correct exit codes

## Implementation Order

1. Lexer: Add U/u suffix parsing and set `is_unsigned` on tokens
2. Parser: Propagate `is_unsigned` from token to AST_INT_LITERAL nodes
3. Codegen: Fix function parameter `is_unsigned` propagation to LocalVar
4. Tests: Rename test file and verify benchmark execution

## Key Decisions

- U/u and L/l suffixes can appear in any order; both are valid
- Case-insensitive suffix parsing (U and u both recognized, L and l both recognized)
- The `is_unsigned` flag on tokens and AST nodes indicates presence of unsigned suffix, not magnitude
- Function parameter signedness is determined by the parameter type declaration, with `is_unsigned` applied to both type and the LocalVar entry
- No changes to ASTNode structure beyond using existing `is_unsigned` field added in stage 40
