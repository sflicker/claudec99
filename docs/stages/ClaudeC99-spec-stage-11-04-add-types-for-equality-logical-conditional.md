# ClaudeC99 Stage 11.4

## Task 
- Extend semantic analysis and code generation so comparison, equality, logical operations and conditional
   tests use the integer type rules added in stage 11.3.

## Goals
- Reuse integer promotions for non-arithmetic expressions
- Reuse common integer type selection for binary comparisons
- Make relational and equality operations behave correctly with `char`, `short`, `int` and `long`.
- Ensure expressions used in `if`, `while`, `for` and `do-while` conditions are evaluated consistently
- Keep results typing for comparison/logical expressions simple and C-like

## Requirements
1. Relational and equality expressions must use promotions
    for these operators
   - `==`
   - `!=`
   - `<`
   - `<=`
   - `>`
   - `>=`
   The operands must:
   1. undergo integer promotion
   2. be converted to a common integer type
   3. be compared using that type
   Examples:
   - `char < short` -> compares as `int`
   - `int == long` -> compares as `long`
2. Comparison result type
    The result type of:
    - equality expressions
    - relational expressions
    - logical `&&`
    - logical `||`
    - logical `!`
   Should be `int`
3. Logical operators must operate on truth values
    For
    - `&&`
    - `||`
    - `!`
    the operands should be interpreted as:
    - zero -> false
    - nonzero -> true
   Result:
    - false -> 0
    - true -> 1
   Result type is `int`
4. Conditional contexts must accept all supported integer types
   Expressions used in:
     - `if`
     - `while`
     - `do-while`
     - `for` test expressions
   should evaluate truthiness using:
     - zero = false
     - nonzero = true
   no explicit case is required
5. Reuse Stage 11.3 type helpers
    This stage should build on helpers such as:
      - promote_integer_type(Type *)
      - common_integer_type(Type *)
6. Type tracking for these expressions
AST nodes or semantic metadata for:
    - relational expressions
    - equality expressions
    - logical expressions
Should carry resulting type `int`

## In Scope
- `==`
- `!=`
- `<`
- `<=`
- `>`
- `>=`
- `&&`
- `||`
- `!`
- conditional test expressions in control flow statements

## Out of Scope
- unsigned types
- casts
- bitwise operators
- shift operators
- typed function parameter passing
- typed function return-value conversions
- ternary `?:`
- diagnostics for suspicious comparisons
- pointer comparisons
- floating point

## Grammar changes
NONE

## Type rules for this stage
Use the same promotion rules from Stage 11.03
- `char` -> `int`
- `short` -> `int`
- `int` -> `int`
- `long` `long`

## Common type for binary comparisons
After promotion
- if either operand is `long`, compare as `long`
- else compare as `int`

## Result type
- `==`, `!=`, `<`, `<=`, `>`, '>=' -> `int`
- `&&`, `||`, `!` -> `int`

## Codegen Expectations
1. Comparison operands
Before comparison:
  - load each operand according to its declared type
  - promote/convert to common type
  - compare using the common type width
For this stage:
  - `int` comparisons use 32-bit integers
  - `long` comparisons use 64-bit integers
2. Comparison results
Comparison codegen should produce:
  - 0 for false
  - 1 for true
stored or left in the normal result register as an `int`
3. Logical operators
Logical operators should produce normalized boolean results:
- `0` or `1`
4. control-flow tests
Branch generation should work for all supported integer expression types, not just `int`.