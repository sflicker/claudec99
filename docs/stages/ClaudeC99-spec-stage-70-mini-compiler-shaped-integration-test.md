# ClaudeC99 stage 70 mini-compiler-shaped integration test

## create a mini compiler-shaped integration test
  - This should stress the features together in a realistic pattern.
  - The goal is not to create a fully functioning compiler (although it could be a very limited one) instead it
    should utilize many of the programming used by this project main code.

## Out-of-scope
  New language feature. **Do not add new features to support this test**. However, bugs may be fixed if
  discovered with existing functionality.

## Test
Create an interation that that includes a small c program that looks like a tiny compiler component
It should have the following features.
  - reads a file or string (The date being read could be a very limited c fragment/program)
  - tokenizes identifiers/numbers/operators
  - stores tokens in a heap array
  - uses structs, enums, string helpers, switch and pointer arithmetic
  - potentially others features used by the code of this project.
  - 

