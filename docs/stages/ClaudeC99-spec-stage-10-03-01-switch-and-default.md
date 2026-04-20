# Claude C99 Stage 10.3-01

## Task:
- Extend the compiler to support a restricted switch statement

## Requirements
- In this stage a partial/restricted version of the switch statement will be added
- This will be functional but will not fully support the standard switch statement.

## Definitions
- The `switch` statement is a multi-way decision that tests whether an expression matches
  one of a number of constant integer values and branches accordingly.

## Tokens
- switch
- default

## Grammar Updates
- note some of these changes are only for this stage

```ebnf
Update:

<statement> ::=  <declaration>
    | <return_statement>
    | <if_statement>
    | <while_statement>
    | <do_while_statement>
    | <for_statement>
    | <switch_statement>
    | <block_statement>
    | <jump_statement>
    | <expression_statement>

ADD:

<switch_statement> ::= "switch" "(" <expression> ")" <switch_body>
<switch_body> ::= "{" <default_section> "}"
<default_section> ::= "default" ":" { <statement> }
```

## Restrictions (Stage 10-03-01)
- The switch body must be a bracd block
- The body must contain exactly one `default` label
- `case` labels are not allowed in this stage
- No statements are allowed outside the `default` section within the switch body

## Semantics
- The switch expression is evaluated once.
- Control transfers directly to the `default` section.
- All statements within the `default` section execute sequentially
- A `break` statement exits the switch.
- If no `break` is encountered, control flows to the end of the switch.