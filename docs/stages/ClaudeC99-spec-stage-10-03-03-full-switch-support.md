# Claude C99 Stage 10.03-03

## Task
- Add standard `switch` behavior for the currently supported subset of C.

## Requirements
- This stage will extends `switch` toward standard C behavior 
  - stacked `case` labels
  - fallthrough between labels is supported
  - unlabeled statements may appear in a switch body
  - control transfers to the matching label, then continues
    normally until `break`, `return` or the end of the switch
- `case` are still restricted to <integer_literal>

## Grammar updates

```ebnf
UPDATE:

<statement> ::=  <declaration>
               | <return_statement>
               | <if_statement>
               | <while_statement>
               | <do_while_statement>
               | <for_statement>
               | <switch_statement>
               | <labeled_statement>
               | <block_statement>
               | <jump_statement>
               | <expression_statement>

<switch_statement> ::= "switch" "(" <expression> ")" <statement>

rename:
<int_literal> -> <integer_literal>

ADD:
<labeled_statement> ::= "case" <constant_expression> ":" <statement>
                      | "default" ":" <statement>
                      
<constant_expression> ::= <integer_literal>                      

REMOVE:
<switch_body>
<switch_section>
<case_section>
<default_section>

```

## Semantics
- The `switch` controlling expression is evaluated exactly once
- Control transfers to the first matching `case` label
  - if no `case` matches, control transfers to `default` if present
  - if no `case` matches and no `default` is present, control flows to the end of the switch
  - After control enters a labeled statement, execution proceeds normally through subsequent 
       statements until:
    - a `break` exits the switch
    - a `return` exits the function
    - control reaches the end of the switch body
  - Fallthrough between cases is supported
  - Multiple stacked `case` labels may apply to the same statement

## Notes 
- For this stage, `<constant_expression>` is restricted to `<integer_literals>`
- The grammar now uses a more standard C-style labeled-statement form
- The compiler may still use an internal representation different from 
  the grammar, as long as behavior matches the stage