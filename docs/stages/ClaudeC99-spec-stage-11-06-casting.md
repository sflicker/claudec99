# Claude C99 Stage 11-06

## Task:
  - Extend the compiler by adding support for explicit casts among the current supported signed integer types: 
    - `char`
    - `short` 
    - `int` 
    - `long`

## Requirements
  - Update parser to support `<cast_expression>` 
  - Update an AST node for explicit casts.
  - Cast AST nodes must track:
    - source expression  
    - source/from type
    - target/to type
  - The type of a cast expression is the target type.
  - Update expression type analysis so casts participate correctly in later arithmetic, comparison, assignment, return and function-call logic.
  - Update codegen to apply the requested conversion.
  - Cast codegen should reuse the same narrowing/widening conversion logic already used for assignment, parameters and returns.

## Tokens update:
  - None

## Grammar update:

```ebnf

<multiplicative_expression> ::= <cast_expression> { ("*" | "/") <cast_expression> }
   
<cast_expression> ::= <unary_expression> 
                      | "(" <integer_type> ")" <cast_expression>   
   
<unary_expression> ::= ( "+" | "-" | "!" | "++" | "--" ) <unary_expression>  
                    | <postfix_expression>

```

## Scope Notes
  - This stage only supports casts to the existing integer types.
  - Casts are explicit conversions.
  - Narrowing casts must truncate.
  - Widening casts must sign-extend.
  - Same-size casts do not change representation but still produce the target type.

## Out of Scope
  - Unsigned casts
  - Pointer casts
  - Function casts
  - Array casts
  - Struct/union casts
  - Full C `<type-name>` parsing
  - Cast-related diagnostics beyond normal parse/semantic errors

## Tests
  - All current tests should pass.
    - Additional tests to cover casts should be added including.
      - `long` to `char` narrowing
      - `long` to `short` narrowing
      - `char` to `long` widening
      - cast expression participating in arithmetic
      - nested casts
      - casts around function-call results
    