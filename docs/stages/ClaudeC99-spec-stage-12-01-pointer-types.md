# Claude C99 Stage 12-01

## Task
  - Extend the compiler type system to support pointer types.

## Requirements
  - Add internal representation to pointer types.
  - A pointer type stores its referenced/base type.
  - Support pointer variable declarations using the current supported integer types:
    - `char *`
    - `short *`
    - `int *`
    - `long *`
  - Pointer variables may be declared without initialization.
  - Pointer variables may be initialized only if the existing expression/type system can already support the initializer.
    - Full address-of/dereference support is deferred.
  - Treat pointer size as 8 bytes for x64-64 codegen
  - Existing integer declarations and all current tests must continue to pass.

## Out of Scope
  - Address-of operator: `&x`
  - Dereference operator: `*p`
  - Pointer assignment through dereference: `*p = value`
  - Pointer Arithmetic
  - Pointer comparisons
  - Arrays
  - Function parameters or returns types involving pointers,

## Grammar updates
```ebnf
    <declaration> ::= <type> <identifier> [ "=" <expression> ] ";"
    
    <type> ::= <integer_type> { "*" }
```

## Parser Notes
  - Parse the base integer type first
  - Then consume zero or more `*` tokens as part of the declaration/type
  - Each `*` wraps the current type in a pointer type.
   - Example
   -   `int **p`   // pointer to pointer of int

## AST/Type system updates
  - A new pointer type should be added to the TypeKind enum
  - A new `Type * base` field should be added to the Type struct
    - Non pointer types can set the base to `NULL`.

## Tests
  - Add existing tests must pass
  - new pointer declaration tests that verify parsing should be added.
    - Example
      ``` 
       int main() {
          int * p;    // parser must handle this
          return 0;   // expect zero
       }
      ```
  - Multiple level pointer declaration tests should also be add
    - Example
      ``` 
        int main() {
          int **s;   //  parser must handle this 
          int ***t:    
          return 0;   // expect zero
    - ```
  - New AST Printer test should be added    


## Done criteria
  - Pointer declarations parse
  - Pointer types are represented internally
  - Pointer locals reserve 8 bytes
  - Existing integer declarations still work
  - All previous tests pass.
  - New tests pass.