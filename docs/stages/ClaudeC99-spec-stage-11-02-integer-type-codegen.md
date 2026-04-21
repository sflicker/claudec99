# ClaudeC99 Stage-11-02

## Task
- Add support in the codegen module for the new type system started in stage 11-01

## Requirements:
- each integer variable must be stored with the proper size and alignment in the generated code
  - char  -> 1 byte
  - short -> 2 bytes
  - int   -> 4 bytes
  - long  -> 8 bytes
- For this stage all integer types will be handled as signed.
- larger variables stored in smaller ones will be truncated
- NOT SUPPORTED FOR THIS STAGE
  - integer promotions
  - usual arithmetic conversions
  - unsigned behavior
  - mixed-type expression rules
  - casts
  - function parameter passing changes
  - return type widening/narrowing beyond local storage behavior
  
