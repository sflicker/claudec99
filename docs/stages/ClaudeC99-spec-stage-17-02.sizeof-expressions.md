# ClaudeC99 Stage-17-02  sizeof expresions

## Task
  - support sizeof expressions

examples
```C
   sizeof x
   sizeof(x)
   sizeof(a + b)
   sizeof(*p)
   sizeof(A[0])
```

## Grammar Updates
```ebnf
<unary_expression> ::= "sizeof" <unary_expression>
                    | "sizeof" "(" <type> ")"
                    | <unary_operator> <unary_expression>
                    | <postfix_expression>
```

## Requirements
  - sizeof expression determines the expression's type but does not evaluate the expression.
  - example
```C
    int x = 1;
    sizeof(x++);    // should not increment x 
  ```
  - For sizeof(expression), the compiler determines the type of the expression using the normal expression typing rules, including integer promotions and usual arithmetic conversions. The expression is not evaluated, and no runtime code is emitted for evaluating it.