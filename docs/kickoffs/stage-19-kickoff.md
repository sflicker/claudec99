# Stage-19 Kickoff: Comma Operator Expressions

## Summary

Stage-19 adds support for the comma operator (`expr1, expr2`) in expression contexts. The comma operator is the lowest-precedence expression operator (lower than assignment), is left-associative, evaluates operands left-to-right, discards the left operand's value, and produces the right operand's value and type. The result is not an lvalue. Critically, commas in function argument lists and parameter lists remain separators, not operators.

**Grammar addition:**
```ebnf
<expression> ::= <assignment_expression> { "," <assignment_expression> }
```

## Tokenizer Changes

None required. TOKEN_COMMA already exists.

## Parser Changes

1. **Rename existing function:** `parse_expression` → `parse_assignment_expression`
   - Update the function body to call `parse_assignment_expression` recursively on the RHS instead of `parse_expression`

2. **Add new top-level `parse_expression`:**
   - Parses one `parse_assignment_expression`
   - Enters a loop for each comma token
   - Builds left-associative `AST_COMMA_EXPR` nodes by parsing the next `parse_assignment_expression` and creating a comma node with the previous expression as left child
   - Returns the final expression

3. **Preserve comma-as-separator in function calls:**
   - Update function-call argument parsing to use `parse_assignment_expression` instead of `parse_expression`
   - This prevents commas in argument lists from being parsed as comma operators

4. **Preserve comma-as-separator in declarations:**
   - Update declaration initializer parsing to use `parse_assignment_expression`
   - (Multi-declarator support is out of scope for this stage)

## AST Changes

1. Add `AST_COMMA_EXPR` to the `ASTNodeType` enum in `include/ast.h`
2. Structure: binary operator with two children `[left_expr, right_expr]`
3. Result type is the type of the right child

## Code Generation Changes

1. **In `codegen_expression`:** Handle `AST_COMMA_EXPR` by:
   - Evaluating the left child (side effects execute, result in rax is discarded)
   - Evaluating the right child (result left in rax)
   - Setting result_type from the right child's type

2. **In `expr_result_type`:** Handle `AST_COMMA_EXPR` by returning the type of the right child

3. **In `sizeof_type_of_expr`:** Handle `AST_COMMA_EXPR` by returning the type of the right child

4. **In pretty printer:** Add `AST_COMMA_EXPR` case to print "CommaExpr:"

## Test Plan

**Valid test cases (approximately 48 total lines):**
- Basic result value: `(1, 2)` → 2
- Left-to-right evaluation: `(x = 1, x + 2)` → 3
- Chain evaluation: `(x = 1, x = x + 2, x = x + 3, x)` → 6
- Comma lower precedence than assignment: `x = 1, y = 2` → x=1, y=2
- Assignment from comma result: `x = (1, 2)` → x=2
- Comma in return statement: `return x = 4, x + 1` → 5
- Comma in if condition: `if (x = 1, x) { ... }` → 10
- Comma in while condition: loop that evaluates to result 6
- Comma as single function argument: `f((a = 3, a + 4))` → 7

**Invalid test cases (expected to fail):**
- Missing right operand: `(1,)` → parse error
- Missing left operand: `(,1)` → parse error
- Trailing comma in function args: `f(1,)` → parse error

## Implementation Notes

- Pay special attention to operator precedence: comma is lower than assignment
- Function call parsing must distinguish between commas as separators (in argument lists) and commas as operators (in single parenthesized expressions passed as arguments)
- Left-associativity means `a, b, c` parses as `(a, b), c`, not `a, (b, c)`
