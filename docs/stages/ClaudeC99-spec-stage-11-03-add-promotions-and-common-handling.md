# ClaudeC99 Stage 11.03

## Task:
- Add semantic and codegen support for integer promotions and common integer type selection in expressions involving 
   `char`, `short`, `int`, and `long`.

## Goals:
- Expressions should no longer treat all operands as if they were already the same type.
- Smaller integer type should be promoted before arithmetic
- Mixed-size integer expressions should evaluate using a consistent common type
- Resulting expression types should be tracked in AST or semantic layer.

## Requirements:
1. Integer promotions
  - `char` and `short` are promoted to `int` when used in arithmetic expressions
  - `int` remains `int`
  - `long` remains `long`
- For this stage:
  - treat all current integer types as signed
  - do not add unsigned behavior
2. Common integer type
  - For binary arithmetic operators, determine a common type for the operands after promotion.
  - For this stage, the common type rules can be:
    - if either operand is `long` result type is `long`
    - else result type is `int`
3. Expression type tracking
  - Expression AST Nodes should carry a resulting type
  - At minimum, should should apply to
    - integer literals
    - identifier expressions
    - unary expressions
    - binary arithmetic expression
    - assign expressions
4. Type arithmetic codegen
  - Arithmetic codegen should use the promoted/common type for the operation
  - Mixed expressions such as:
    ```
       char a;
       long b;
       return a + b;
    ```
    should evaluate as `long`
5. Assignment conversion
  - After evaluating the right-hand side expression, convert to the destination type when storing
  - This should reuse the typed store/truncation behavior already in stage 11.2
6. Integer literals
  - For this stage, integer literals continue to default to `int`

## In Scope
- `+`
- `-`
- `*`
- `/`
- unary `+`
- unary `-`
- assignment to local variables
- expression type computation
- mixed arithmetic among `char`, `short`, `int`, `long`

## Out of Scope
- unsigned types
- casts
- literal suffixes like `L`
- `long long`
- bitwise operators if not already type-aware
- comparisons/logical operators
- function parameter type changes
- function return type changes
- diagnostics for narrowing conversions
- overflow checking

## Type Rules for This Stage
### Promotions
- `char -> int`
- `short -> int`
- `int -> int`
- `long -> long`

### Common Type
After Promotion
- `long op long -> long`
- `long op int -> long`
- `int op long' -> long`
- `int op int -> int`

### Assignment
- RHS is evaluated using common/promoted type rules
- final value is converted to the LHS declared type on store

## Grammar changes
NONE

