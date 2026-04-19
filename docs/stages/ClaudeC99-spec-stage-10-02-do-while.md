# Claude C99 Stage 10.2

## Task:
- Extend the compiler to support do..while statements.

## Definitions
- a do..while statement is a loop construct that evaluates its condition after executing the body.
  Therefore, the body is always executed at least once.

## Tokens to Add
- `do`
- do not need to add a token for `while` as it is already handled.

## Grammar Changes
The full grammar is in docs/grammar.md

Add: 
```ebnf
<do_while_statement> ::= "do" <statement> "while" "(" <expression> ")" ";" 
```

Update:
```ebnf
<statement> ::=  <declaration>
| <return_statement>
| <if_statement>
| <while_statement>
| <do_while_statement>
| <for_statement>
| <block_statement>
| <jump_statement>
| <expression_statement>
```

## Control flow
- for a `do while` the control is:
  - execute the body <statement>
  - evaluate the expression
    - true -> continue with next iteration executing the body <statement>
    - false -> loop terminates
- `do..while` statement establishes a loop context for break and continue, just like `while` and `for`.
- `break` - a break inside the `do..while` body will terminate the loop.
- `continue` - transfers control to the loop condition check, not directly to the start of the body.

## Notes
