# Claude C99 Stage 10.3-02

## Task:
- Extend the restricted switch statement to support `case` labels and case-based dispatch

## Requirements
- In this stage, `switch` will be extended beyond the `default` only from from stage 10-03-01;
- A restricted but functional version of `case` support will be added
- The compiler must generate working code for `switch` statements that dispatch to 
   matching `case` sections.
- The switch expression must be evaluated exactly once.
- `break` inside a `switch` section must exit the switch
- This stage is still a restricted subset of full c `switch` behavior

## Definitions
- A `case` label introduces a labeled section within a switch
- The controlling expression of the `switch` is compared against each `case` value
- If a matching `case` is found, control transfers to that section
- If no `case` matches, control transfers to the `default` section if present
- If no `case` matches and no `default` is present, control flows to the end of the switch

## Tokens
- `case`

## Grammar Updates

```ebnf

UPDATE:
<switch_body> ::= "{" <switch_section> { <switch_section } "}"

ADD:
<switch_section> ::= <case_section> | <default_section>

<case_section> ::= "case" <integer_literal> ":" { <statement> }

Full affected grammar for this stage:

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

<switch_statement> ::= "switch" "(" <expression> ")" <switch_body>

<switch_body> ::= "{" <switch_section> { <switch_section> } "}"

<switch_section> ::= <case_section> | <default_section>

<case_section> ::= "case" <integer_literal> ":" { <statement> }

<default_section> ::= "default" ":" { <statement> }

```

## Restrictions (Stage 10-03-02)
- The switch body must be a braced body
- The switch body must consist of only `case` and `default` sections
- No unlabeled statements are allowed directly in the switch body outside a section
- `case` values must be integer literals in this stage
- At most one `default` section is allowed
- Stacked labels are not supported in this stage
  - example NOT supported:
     `
      case 1:
      case 2:
        x = 5;
     `
- Tests for this stage must not rely on fallthrough behavior
- Each section should end with `break`, `return` or reach the end of the switch body
- Duplicate `case` values do not need to be diagnosed yet unless this is easy to add

## Semantics
- The switch controlling expression is evaluated exactly once
- The resulting value is compared against each `case` value in source order
- If a matching `case` is found, control transfers to that section
- If no `case` matches and a `default` section exists control transfers to `default`
- If no `case` matches and no `default` section exists, control transfers to the end of the switch
- Statements inside the selected section execute sequentially
- A `break` statement exists the switch
- A `return` statement behaves normally and exits the function

## Implementation Notes
- Reuse the existing `switch` support from Stage 10-03-01
- Extend the AST for `switch` so that the switch body is represented as a sequence of labeled sections
- Each section should record:
  - whether it is a `case` or `default`
  - the integer literal value for a case
  - the list of statements in that section
- The parser should parse statements in a section until the next `case`, `default` or closing `}` of the switch body
- Code generation may use a simple linear compare-and-branch sequence
- Do not implement jump table optimization in this stage

## Suggested AST Shape
This is only a suggestion. Use the project's naming conventions
- SwitchStmt
  - Controlling expression
  - list of switch sections
- SwitchSection
  - kind: `case` or `default`
  - case value if kind is `case`
  - list of statements

## Code Generation Expectations
A straightforward implementation is acceptable.
1. Evaluate the switch expression once
2. Compare it against each `case` value
3. Jump to the matching section label if found
4. if no match is found:
   - jump to the `default` section if present
   - otherwise jump to the switch end label
5. Emit code for each section
6. `break` jumps to the switch end label
No optimization is required

## Test requirements
A number of tests for valid cases have already been added.
If there are any testing gaps add additional ones

## Out of Scope for this stage
- full C labeled-statement grammar
- arbitrary unlabel statements directly in the switch body
- stacked `case` labels
- required fallthrough support
- constant expressions beyond integer literals for `case`
- duplicate-case diagnostics as a required feature
- jump table optimization
- 
