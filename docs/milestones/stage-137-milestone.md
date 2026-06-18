# Milestone Summary

## Stage 137: Function Return Function Pointers - Complete

stage-137 ships support for the `int (*get_adder())(int)` declarator form, allowing functions to return function pointers. This resolves spec issue **CC99-010**.

- **Parser**: Extended `ParsedDeclarator` with `is_func_returning_fp`, `own_param_types`, `own_param_count`, and `own_is_no_prototype` fields to track the inner function's signature. The `parse_declarator` function now parses the `(*name())(params)` form fully instead of rejecting it as unsupported. A guard on `inner_stars == 0` remains to reject truly-invalid direct-function returns (e.g., `int (f())(int)`).
- **Semantic**: In `parse_external_declaration`, added an `is_func_returning_fp` branch that constructs the nested pointer-to-function type correctly, creates `AST_FUNCTION_DECL` with `decl_type = TYPE_POINTER`, registers the function, and optionally parses its body. Fixed a pre-existing bug where typedef'd pointer return types were not recognized via `func->full_type` assignment condition.
- **Tests**: Removed 1 invalid test (now valid: functions returning function pointers). Added 4 valid tests and 1 new invalid test covering: basic `int (*f())(int)` with direct-call syntax, assignment to function-pointer variables, typedef'd function-pointer spellings, and the correctly-rejected direct-function form.
- **Codegen**: No changes required; function pointers and indirect calls already work correctly.
- **Results**: Full test suite: 1965 tests pass (1282 valid, 259 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). Self-host C0→C1→C2 verified with all tests passing at every stage; no source changes needed during bootstrap.
- **Notes**: The parser's approach of storing the inner function's parameter list on `ParsedDeclarator` enables correct type construction without duplicating the function signature logic. The `inner_stars == 0` guard prevents confusion with the invalid-but-superficially-similar `int (f())(int)` form (parenthesized function declarator).
