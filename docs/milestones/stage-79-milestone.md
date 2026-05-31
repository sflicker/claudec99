# Milestone Summary

## stage-79: General Lvalue Compound Assignment - Complete

stage-79 extends compound-assignment operators (+=, -=, *=, /=, %=, <<=, >>=, &=, ^=, |=) to work on any modifiable lvalue, not only bare identifiers.

- Tokenizer: No changes.
- Grammar/Parser: parse_assignment_expression (in parser.c) now recognizes compound-assignment operators after general postfix expressions (not only identifiers). For each compound operator, the desugaring produces an AST_ASSIGNMENT with an AST_BINARY_OP rvalue: `lhs op= rhs` → `lhs = lhs op rhs`. The lvalue-validity check now applies to compound forms.
- AST/Semantics: Added ast_clone() function to deep-copy AST subtrees, so a lvalue can serve both as the assignment target and in the binary operand without node aliasing.
- Codegen: No changes. The existing two-child AST_ASSIGNMENT form and binary-op codegen already handle the desugared form.
- Tests: 3 new valid tests (each returning 42): struct-pointer arrow access (`p->cap *= 2`), array-element member access (`tokens[i].kind += 2`), pointer dereference (`*p *= 2`). Full suite: 768 valid, 231 invalid, 71 integration, 43 print-AST, 99 print-tokens, 21 print-asm; 1233 total, all pass.
- Docs: README.md updated to reflect stage 79 completion and general-lvalue compound assignment support; grammar.md requires no change (assignment syntax already covers general lvalues).
- Notes: The desugaring evaluates the lvalue twice (e.g., `p->cap *= 2` becomes `p->cap = p->cap * 2`), matching the spec's conceptual model.
