# Stage 18 Kickoff: Conditional Operator

## Summary of the Spec

Stage 18 adds support for the ternary conditional operator `?:` with the syntax `condition ? expr_true : expr_false`.

Key semantics:
- Condition is evaluated first; only the selected branch (true or false) evaluates.
- Condition must be scalar (integer or pointer).
- Result type determination: both integer operands → common integer type; both pointer operands → pointer type; one pointer and `0` → pointer type.
- Conditional expression is not an lvalue.
- Right-associative; lower precedence than logical OR, higher than assignment.

## Required Tokenizer Changes

1. Add `TOKEN_QUESTION` for `?` to `include/token.h`
2. `TOKEN_COLON` already exists
3. Add lexer case in `src/lexer.c` to map `?` → `TOKEN_QUESTION`

## Required Parser Changes

1. Add `parse_conditional()` function in `src/parser.c`
2. Update grammar (right-associative):
   ```
   <conditional_expression> ::= <logical_or_expression>
                              | <logical_or_expression> "?" <expression> ":" <conditional_expression>
   ```
3. Place `parse_conditional()` in precedence hierarchy between `parse_logical_or()` and `parse_expression()`
4. Update `parse_expression()` to call `parse_conditional()` instead of `parse_logical_or()`
5. Error handling: detect missing true expression, missing `:`, missing false expression

## Required AST Changes

1. Add `AST_CONDITIONAL_EXPR` node type to `include/ast.h`
2. Node has 3 children:
   - `[0]` = condition
   - `[1]` = true_expr
   - `[2]` = false_expr

## Required Code Generation Changes

1. Handle `AST_CONDITIONAL_EXPR` in `codegen_expression()` in `src/codegen.c`
2. Emit sequence:
   - Evaluate condition (sets `result_type`)
   - Compare: `cmp rax/eax, 0` then `je .L_cond_false_N`
   - True branch: evaluate `true_expr`, `jmp .L_cond_end_N`
   - `.L_cond_false_N:` evaluate `false_expr`
   - `.L_cond_end_N:`
3. Determine result type: integer/integer → common int type; pointer/pointer → pointer type
4. Use `cg->label_count++` to generate unique label N

## Pretty Printer Changes

Add `AST_CONDITIONAL_EXPR` case to `src/ast_pretty_printer.c` to print `"Conditional:\n"`.

## Test Plan

### Valid Tests (9 cases)

1. Basic true: `1 ? 10 : 20` → 10
2. Basic false: `0 ? 10 : 20` → 20
3. Condition from comparison: `x > 3 ? 1 : 0` → 1
4. Only true branch evaluated (division by zero in false branch)
5. Only false branch evaluated (division by zero in true branch)
6. Right associativity: `0 ? 1 : 0 ? 2 : 3` → 3
7. Precedence with logical OR: `0 || 1 ? 10 : 20` → 10
8. Precedence with assignment: `x = 1 ? 10 : 20` → 10
9. Nested conditionals: `1 ? 0 ? 2 : 3 : 4` → 3
10. Pointer condition: `p ? *p : 0` where `p` is a pointer

### Invalid Tests (4 cases)

1. Missing true expression: `1 ? : 2`
2. Missing colon: `1 ? 2`
3. Missing false expression: `1 ? 2 :`
4. Missing condition: `? 1 : 2`

## Notes on Spec Issues

- Grammar notation typo in spec (line 33): `::` should be `::=`
- Test typo in spec (line 122): `return x : (1/y) : 20;` should be `return x ? (1/y) : 20;`
