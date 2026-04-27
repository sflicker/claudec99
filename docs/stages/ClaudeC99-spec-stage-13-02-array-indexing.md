# ClaudeC99 Stage 13.02

## Task
  - Add support for array subscript expressions: `a[i]`

## Require
  - Parse postfix subscript expressions
  - Add AST node for array indexing
  - Support reading array elements
  - Support assigning to array elements
  - index must be integer type
  - Base must be array type
  - Result is an lvalue of the array element type

## Grammar Updates
```ebnf
    <postfix_expression> ::= <primary_expression> 
                               { "[" <expression> "]" 
                                 | ++" 
                                 | "--" } 
```

## Semantics
   `a[i]`
   
  is treated as 
    `(base address of a + i *sizeof(element_type))`
  
## Out of Scope
  - pointer indexing: `p[i]`
  - Array-to-pointer decay
  - Passing array to functions
  - Pointer arithmetic
  - Multidimensional Arrays

## Tests
  ```C 
      int main() {
         int a[3];
         a[0] = 4;
         a[1] = 5;
         a[2] = 6;
         return a[0] + a[1] + a[2]; // expect 15
      }
  ```

  ```C
      int main() { 
          char a[2];
          a[0] = 10;
          a[1] = 20;
          return a[0] + a[1];  // expect 30
      }
  ```

  ```C
      int main() {
          long a[2];
          a[0] = 40;
          a[1] = 2;
          return a[0] + a[1];   // expect 42
      }      
  ```
