# ClaudeC99 Stage 14-02

## Task
  - add parser and AST support for string literals

## Requirements
  - Add an AST node kind for string literals
  - Parse 'TOKEN_STRING_LITERAL' as a primary expression
  - Preserve the string literal token value in the AST node
  - Preserve the string literal byte length in the AST node
  - Update the AST pretty-printer to display string literal nodes
  - Add print-AST tests for string literals
  - Existing tests must continue to pass

## Grammar updates

```ebnf
    <primary_expression> ::= <integer_literal>
                         | <string_literal>
                         | <identifier>
                         | <function_call>
                         | "(" <expression> ")"
                         
    <string_literal> ::= TOKEN_STRING_LITERAL                         
```

## Type Rule
For now, a string literal should be treated as an expression whose logical type is:
```C
    char[N]
```
   Where N is the literal byte length plus one for the null terminator.

## Tests
Only print_ast tests should be added. Here a few examples

Basic example
```C
    int main() {
        return "hi";
    }
```

Empty string
```C
    int main() {
        return "";
    }
```

## Out of Scope
  - Emitting string literals into .rodata
  - Generating addresses for string literals
  - Assigning string literals to `char *`
  - Escape sequences
  - Char array initialization from string literals
  - adjacent string literal concatenation
  - Runtime tests involving string literals
