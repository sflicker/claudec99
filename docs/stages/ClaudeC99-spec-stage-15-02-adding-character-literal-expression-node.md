# ClaudeC99 Stage 15-02

## Task
  - Add a character literal expression node

## Requirements
  - Add a character literal expression node
    - `AST_CHAR_LITERAL`

## Grammar Update
```ebnf
INCLUDE <char_literal> to be accepted by <primary_expression>
 
<primary_expression> ::= <integer_literal>
                         | <string_literal>
                         | <char_literal>
                         | <identifier>
                         | <function_call>
                         | "(" <expression> ")"
                         
<char_literal> is already in the grammar
Other adjustments maybe be necessary                         
                                                  
```