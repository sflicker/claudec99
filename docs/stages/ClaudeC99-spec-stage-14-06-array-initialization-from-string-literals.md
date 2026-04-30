# ClaudeC99 Stage 14-06

## Task
  - Support initializing local `char`arrays from string literals.

## Requirements


## Grammar Update

//TODO pick one of these two forms

<declaration> ::= <type> <identifier> [ "=" <expression> ] ";"
| <type> <identifier> "[" <integer_literal> "]" [ "=" <expression> ] ";"
| "char" <identifier> "[" "]" "=" <string_literal> ";"

or

<declaration> ::= <type> <identifier> [ "[" [ <integer_literal> ] "]" ] [ "=" <expression> ] ";"