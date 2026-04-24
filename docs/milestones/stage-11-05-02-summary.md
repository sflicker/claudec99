# Stage-11-05-02 Milestone: Typed Function Signatures and Call Semantics

Extended the parser's function-symbol table so each `FuncSig` stores
the declared return type and an ordered list of parameter types, and
plumbed that return type through to function-call expressions so they
participate in the existing type system.

## What was completed
- `FuncSig` (include/parser.h) gained `return_type` and a
  `param_types[FUNC_MAX_PARAMS]` array (max 16 parameters).
- `parser_register_function` now accepts the return type and parameter
  types and records them on first registration; existing arity and
  duplicate-definition checks are unchanged.
- `parse_function_decl` collects parameter types from the child
  `AST_PARAM` nodes and registers the full signature.
- `parse_primary` stamps the resolved callee's declared return type
  onto the `AST_FUNCTION_CALL` node's `decl_type` at call-site
  resolution, reusing the existing AST field.
- `expr_result_type` and the AST_FUNCTION_CALL arm of
  `codegen_expression` now read `node->decl_type` instead of hardcoding
  `TYPE_INT`. No new conversions or instructions are emitted — the
  existing promotion / common-type / long-path plumbing picks up the
  new type automatically.
- Added three new valid tests:
  `test_call_returns_long__42.c`,
  `test_call_long_plus_char__7.c` (Section 6 spec example),
  `test_call_short_params_allowed__3.c` (parameter types stored but
  not enforced).

## Scope limits (per spec)
No grammar changes, no AST struct changes, no tokenizer changes, no
AST-pretty-printer changes, no argument-to-parameter or return-value
conversions, no signature-compatibility checks between declarations and
definitions beyond the existing arity check.

## Test results
All 193 tests pass (166 valid + 14 invalid + 13 print_ast). No
regressions.
