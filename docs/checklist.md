# ClaudeC99 Feature Checklist

## Stage 1 - Minimal Compiler

- [x] Parse single function
- [x] Support return <int_literal>;
- [x] Tokenizer
	- [x] int
	- [x] return
	- [x] identifiers
	- [x] integer literals
	- [x] punctuation ( ) { } ;
- [x] AST
	- [x] program node
	- [x] function node
	- [x] return statement node
	- [x] integer literal node
- [x] Code Generation
	- [x] emit constant return value
- [x] Basic Test Harness

## Stage 2 - Arithmetic Expressions

- [x] Add Tokens: + - * / ( )
- [x] Extend Grammar for expressions
  - [x] addition/subtraction
  - [x] multiplication/divison
  - [x] paratheses
- [x] AST
  - Binary Expressions nodes
- [x] Parser
  - [x] precedence handling ( term, factor )
- [x] Code generation
  - [x] arithmetic operations
- [x] tests for expressions evaluation