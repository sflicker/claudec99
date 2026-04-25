# Claude C99 Stage 12.4

## Task
  - Extend pointer support to include function parameters and pointer arguments

## Example
```C 
    int read(int * p) {       // accepts a pointer parameter
        return *p;
    }
    
    int main() {
      int x = 7;
      return read(&x);       // passing an address/pointer argument.
    }                        // expect 7
```

## Requirements
  - Support pointer types in function parameter declarations
  - Support passing pointer-valued expressions as function arguments
  - Support passing address-of expressions such as `&x` to pointer arguments
  - Store pointer parameters as 8-byte local values in callee.
  - Allow dereferencing pointer arguments inside the callee.
  - Type-check caller arguments against callee parameter types
    - `int *` argument may be passed to `int *` parameter.
    - `char *` argument may be passed to `char *` parameter.
    - Mismatched pointer base types should be rejected for now
    - Passing an integer where a pointer is expected should be rejected
    - Passing a pointer where a integer is expected should be rejected 

## Grammar Updates
```ebnf
    <parameter_declaration> ::= <type> <identifer>
```

## Out of Scope
  - pointer return types
  - pointer arithmetic
  - null pointer constants
  - arrays
  - Pointer type conversion/casts between incompatible pointer types

## Testing
  - All current tests must pass (or adjusted if changes deem necessary)
  - New tests should be added where appropriate. Including:

  - **Pointer parameter read**
    ```C 
      int read(int *p) {
          return *p;
      } 
    
      int main() {
         int x = 7;
         return read(&x);    // expect 7
      }     
    ```

  - **Pointer parameter write**
   ```C 
      // void not support so use int
      int set(int *p) {
          *p = 9;
          return 0;
      }
      
      int main() {
          int x = 1;
          set(&x);       // return value ignored
          return x;      // expect 9
      }
  ```

  - **Nested pointer parameter**
  ```C 
      int read2(int **pp) {
          return **p;
      }
      
      int main() {
          int x = 11;
          int *p = &x;
          return read2(&p);   // expect 11
      }   
  ```

  - **Invalid: Integer passed to pointer parameter**
  ```C 
      int read(int *p) {
          return *p;
      } 
      
      int main(*) {
          int x = 7;
          return read(7);     // Compile Error
  ```

  - **Invalid: Pointer passed to integer parameter**
    ```C 
       int f(int x) {
           return x;
       }
    
       int main() {
          int x = 7;
          return f(&x);       // Compile Error
       }
    ```
    
  = **Invalid:: Mismatched pointer type**
    ```C 

        int read(int * p) {
            return *p;
        }

        int main() {
           char c = 7;
           return read(&c);     // Compile Error
        }
    ```

  ## Done Criteria
    - Pointer parameters parse
    - Pointer arguments type-check correctly
    - Pointer parameters are stored as 8 byte local values
    - Dereferencing pointer parameters works.
    - All Tests pass