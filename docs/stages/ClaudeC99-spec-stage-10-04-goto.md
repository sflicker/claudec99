# Claude C99 Stage 10.04

## Task
- Add support for the to `goto` statement.

## Requirements 
- This stage will add support for the `goto` statement
- a `goto` will cause execution to move to a different label `statement`.
- a `goto` can only jump to another location with in the function.
- label names must be unique within the same function
- For codegen label names must be unique within the assembly output not just within a function.
- if duplicate labels are found within a function this is a compile error.
- if a `goto` label is missing this is a compile error
- no computed goto
- no label addresses

## Token updates
- `goto`

** Grammar updates
```ebnf
UPDATE:
    <labeled_statement> ::= <identifier> ":" <statement>
                      | "case" <constant_expression> ":"
                       <statement>
                      | "default" ":" <statement>

    <jump_statement> ::= "continue" ";"
                     | "break" ";"
                     | "goto" <identifier> ";"
```

## Tests
- some tests have already been provided for valid cases.
- additional tests for valid cases may be created to fill testing gaps.
- additional tests for invalid, and print_ast should be created.
