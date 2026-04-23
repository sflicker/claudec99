# ClaudeC99 Stage 11.5-1

## Task
- Extend the grammar, parser and AST representation to support integer types for function parameters and return types.

## Goals
- Update only the grammar, parser, AST representation and AST printer
- Codegen should remain unchanged
- Existing valid tests should remain unchanged and still pass
- AST print tests will be updated to reflect the grammar changes

## Grammar updates

```ebnf

    <function> ::= <integer_type> <identifier> "(" [ <parameter_list> ] ")" ( <block_statement> | ";" )
    
    <parameter_declaration> ::= <integer_type> <identifier>

```

## Requirements
- The parser will be updated to handle the new grammar
- Function AST nodes must store the declared return type
- Parameter AST nodes must store the declared parameter type
- Print AST tests will be updated to cover the updates
- Existing function call arity checks will remain in place
- For this stage, do not add parameter type checking for function calls
- For this stage, do not add return type checking or return-value conversion
- No changes to codegen
 
## Out of Scope 
- Function call expression typing
- Argument-to-parameter conversions
- Return-value conversions
- Function signature compatibility checking beyond existing arity checks
- Any codegen changes