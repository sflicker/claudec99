# ClaudeC99 Stage 16-05

## Task
  - add remaining compound assignment operators
```C
    *=
    /=
    %=
    <<=
    >>=
    &=
    ^=
    |= 
  ```

## Token Update
Add new tokens for the remaining assignment operators
```C
    *=
    /=
    %=
    <<=
    >>=
    &=
    ^=
    |= 
  ```

## Grammar Update
```ebnf
<assignment_operator> ::= "=" |"+=" | "-=" | "*=" | "/=" |
                          "&=" | "<<=" | ">>=" | "&=" | "^=" | "|="          
```

## Tests
Generate Appropriate Tests
