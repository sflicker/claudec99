# ClaudeC99 Stage 40 - unsigned integer types and size_t

## Goal
  - add support for `unsigned` integer types
  - add support for just `unsigned` be eqv to `unsigned int`
These should all be allowed
```C
       unsigned x;
       unsigned char c;
       unsigned short s;
       unsigned int i;
       unsigned long l; 
  ```
Other potential variations are out of scope
```C
      long unsigned x;   // INVALID
      int unsigned x;    // INVALID
      unsigned long int x;  // INVALID
      signed int x;   // INVALID
      signed x;      // INVALID
      long long x;     // INVALID
```

## Tokens
  Add a token for the keyword `unsigned`
  
## Grammar Changes
```ebnf
<integer_type> ::= { "unsigned" } (char" | "short" | "int" | "long")
                  | "unsigned"

```

## Requirements
  - Add unsigned as a type specifier
  - Support unsigned char/short/int/long and plain unsigned
  - Store signedness in integer types
  - implement assignment/conversion behavior for unsigned objects
  - use unsigned codegen for division, remainer, relational comparisons, right shift, and small type loads
  - allow `typedef unsigned long size_t` in tests
  - If an arithmetic or relational operation mixes signed int and unsigned int, convert the signed 
    operand to unsigned int first. More complete rules
    - Integer promotions happen first
    - If both operands have the same signedness, convert to the higher-rank type
    - if one is unsigned and the other is signed
      - if the unsigned type's rank is greater than or equal to the signed type's rank
        convert the signed operand to unsigned.
      - Else, if the signed type can represent all values of the unsigned type
        convert the unsigned operand to signed
      - Otherwise, convert both to the unsigned version of the signed type
       
## Out of Scope
  - All declaration-specifier order permutations, e.g. long unsigned int
  - Unsigned Integer literal suffixes u/U
  - Full usual arithmetic conversions in every edge case
  - Warnings for signed/unsigned mixing

## Codegen
  Use proper asm instructions when dealing with unsigned 
  Use proper extension when widening.
    - for signed use sign-extension
    - for unsigned use zero-extension

## Tests
```C
    int main() {
        int s = -1;
        unsigned int u = 1;
        if (s < u) 
            return 1;
            
        return 42;        // expect 42
```
Add other tests to cover new functionality and invalid cases

