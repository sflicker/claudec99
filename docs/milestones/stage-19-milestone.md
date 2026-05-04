# Milestone Summary

## Stage-19: Comma Operator Expressions - Complete

stage-19 ships the comma operator (`expr1, expr2`), which evaluates the left operand, discards its result, then evaluates and returns the right operand.
- Tokenizer: No changes (comma token already exists).
- Grammar/Parser: Renamed `parse_expression` to `parse_assignment_expression`; added new `parse_expression` to handle comma sequences at the lowest precedence (below assignment). Function call arguments and initializers now use `parse_assignment_expression` to preserve comma-as-separator semantics. Comma expressions are left-associative.
- AST/Semantics: Added `AST_COMMA_EXPR` node type.
- Codegen: Added code generation for comma expressions: evaluate left operand (discard), evaluate right operand (use result); result type is the right operand's type.
- Tests: 12 new tests (9 valid, 3 invalid). Full suite 609/609 pass (up from 597).
- Docs: Updated `docs/grammar.md` to include comma operator rule.
- Notes: Comma operator is the lowest-precedence operator (below assignment) and is not an lvalue. The feature correctly distinguishes comma-as-operator from comma-as-separator in function arguments and initializers.
