# Claude C99 Stage 9.2

## Task
Refactor the top-level grammar to introduce <translation-unit> and <external-declaration> in preparation for multi-function support.

This stage is grammar-only and does not yet introduce new language feature beyond what is already support.

## Goals
- Replace `<program>` with `<translation-unit>`
- Introduce `<external-declaration>` as a wrapper for top-level constructs
- Keep behavior identical to previous stage (single function still works)
- Lay groundwork for future stages that will support multiple functions.
- Do not add any language features

## Updated Grammar

```ebnf

# Claude C99 Grammar (Current)

<translation-unit> ::= <external-declaration>

<external-declaration> ::= <function>

<function> ::= "int" <identifier> "(" ")" <block_statement>

<block_statement> ::= "{" { <statement> } "}"

<statement> ::=  <declaration>
                    | <return_statement>
                    | <if_statement>
                    | <while_statement>
                    | <for_statement>
                    | <block_statement>
                    | <expression_statement>

<declaration> ::= "int" <identifier> [ "=" <expression> ] ";"

<return_statement> ::= "return" <expression> ";"

<if_statement> ::= "if" "(" <expression> ")" <statement> [ "else" <statement> ]

<while_statement> ::= "while" "(" <expression> ")" <statement>

<for_statement> ::= "for" "(" [<expression>] ";" [<expression>] ";" [<expression>] ")" <statement>

<expression_statement> ::= <expression> ";"

<expression> ::= <assignment_expression>

<assignment_expression> ::= <identifier> "=" <assignment_expression>
| <identifier> "+=" <assignment_expression>
| <identifier> "-=" <assignment_expression>
| <logical_or_expression>

<logical_or_expression> ::= <logical_and_expression> { "||" <logical_and_expression> }

<logical_and_expression> ::= <equality_expression> { "&&" <equality_expression> }

<equality_expression> ::= <relational_expression> { ("==" | "!=") <relational_expression> }

<relational_expression> ::= <additive_expression> { ( "<" | "<=" | ">" | ">=") <additive_expression> }

<additive_expression> ::= <multiplicative_expression> { ("+" | "-") <multiplicative_expression> }

<multiplicative_expression> ::= <unary_expression> { ("*" | "/") <unary_expression> }

<unary_expression> ::= ( "+" | "-" | "!" | "++" | "--" ) <unary_expression>  
| <postfix_expression>

<postfix_expression> ::= <primary_expression> { "++" | "--" }

<primary_expression> ::= <int_literal>
| <identifier>
| "(" <expression> ")"

<identifier> ::= [a-zA-Z_][a-zA-Z0-9_]*

<int_literal> ::= [0-9]+c

```

## Implementation Notes
- This stage is intentionally minimal.
- Update all references from `program` -> `translation-unit`
  - including:
  - parser entry point
  - AST Node names (if applicable)
  - code generation entry point
- Introduce parsing functions:
  - parse_translation_unit()
  - parse_external_declaration()

## Constraints (for this stage)
- No support for multiple functions yet
- No function declarations (prototypes)
- No global variables
- No new tokens or grammar rules beyond those shown above.

## Testing
- No existing test must pass with modification
- Behavior must remain identical to previous stage

## Rationale
- This change introduces the standard C complication structure:
  - `<translation-unit>` represents a full source file
  - `<external-declaration>` represents top-level constructs.
  
