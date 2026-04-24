# Stage-11-05-02 Kickoff: Typed Function Signatures and Call Semantics

## Spec Summary
Stage 11-05-02 adds semantic awareness of function signatures on top of
the typed grammar introduced in 11-05-01. Each function symbol must now
carry its return type and an ordered list of parameter types.
Function-call expressions must be typed with the declared return type of
the callee so existing type rules (promotion, common-type selection,
arithmetic, comparison, logical) pick them up automatically. The arity
check remains the only enforced signature check. No grammar changes, no
codegen restructuring, no argument-to-parameter or return-value
conversions, and no signature-compatibility checks between decl/def.

## What Must Change From Stage 11-05-01
- `FuncSig` (parser) gains `TypeKind return_type` and an ordered
  `param_types[]` array.
- `parse_function_decl` + `parser_register_function` capture and record
  those fields.
- When `parse_primary` resolves a call it must stamp the callee's
  declared return type onto the AST_FUNCTION_CALL node (reusing
  `decl_type`).
- `expr_result_type` and the AST_FUNCTION_CALL branch of
  `codegen_expression` read that declared return type instead of
  hardcoding `TYPE_INT`.

## Out of Scope
Argument-to-parameter conversions, return-value conversions, strict
parameter type checking, function-signature compatibility checks between
declarations and definitions, ABI/calling-convention changes, and any
diagnostics beyond the existing arity error.

## Spec Notes / Ambiguity
- **Typo in Requirement 1 example**: the declaration
  `long sum(char a, char b)` is shown producing signature
  `sum : (char, short) -> long`. The source declares both params as
  `char`; the signature line should read `(char, char) -> long`. The
  intent — store the actual declared parameter types — is clear.
- **"Keep codegen unchanged" tension**: Requirement 6 requires
  `x + f()` to select `long` as the common type when `f()` returns
  `long`. Today `codegen_expression` hardcodes `result_type = TYPE_INT`
  for AST_FUNCTION_CALL and `expr_result_type` defaults calls to
  `TYPE_INT`. Correctly propagating the return type is therefore a
  one-line change in each of those two spots. The emitted call site
  (arg marshalling, `call` instruction) is unchanged; the downstream
  `movsxd`/`rax` plumbing is already in place from earlier long-typing
  stages and engages naturally once the call is tagged long. Reading
  "no codegen changes" as "no new conversions, no new instructions".
- Spec item 7 says AST updates for the call node are optional. Reusing
  the existing `ASTNode.decl_type` field avoids new struct members.

## Planned Changes
- **Tokenizer**: none.
- **Parser** (`include/parser.h`, `src/parser.c`):
  - Add `FUNC_MAX_PARAMS` constant (16).
  - Extend `FuncSig` with `TypeKind return_type` and
    `TypeKind param_types[FUNC_MAX_PARAMS]`.
  - Thread return and param types through `parser_register_function`.
  - Populate them from `parse_function_decl` (return type) and each
    `AST_PARAM` child's `decl_type` (parameter types).
  - In `parse_primary`, after resolving the `FuncSig` for a call, set
    `call->decl_type = sig->return_type;`.
- **AST** (`include/ast.h`): no new fields (reuse `decl_type`).
- **Codegen** (`src/codegen.c`):
  - `expr_result_type` AST_FUNCTION_CALL: return `node->decl_type`.
  - `codegen_expression` AST_FUNCTION_CALL: set
    `node->result_type = node->decl_type;`.
- **AST pretty printer**: none — the spec requires no new printed
  surface.
- **Grammar doc** (`docs/grammar.md`): none — no grammar changes.
- **Tests**:
  - `test_call_returns_long__42.c` — plain `return f()` where `f`
    returns `long`.
  - `test_call_long_plus_char__7.c` — the Section 6 mixed-arithmetic
    example (`x + f()` with char + long call).
  - `test_call_short_params_allowed__3.c` — parameters-stored-not-
    enforced case from Semantic Behavior.
  - Arity error already covered by `test_invalid_8`.
- **Commit**: one commit once green.
