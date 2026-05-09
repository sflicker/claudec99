# ClaudeC99 Stage 28-01 simple typedef names

## Task
  - Add support for simple `typedef` aliases for existing integer scalar types
  - 
Examples

Block scope object using a typedef name
```C
     typedef int I;
     
     int main() {
         I x = 3;
         return x;
     } 
  ```

File scope object using a typedef name
```C
    typedef int I;
    I x = 4;
    
    int main() {
        return x;
    }
```

Typedef used in function signature

```C
    typedef int I;
    
    I add(I a, I b) {
        return a + b;
    }
    
    int main() {
        return add(2,3);    // expect 5
    }
```

## Token to add
 `typedef`

## Grammar Update
```ebnf
    <storage_class_specifier> ::= "extern" | "static" | "typedef"
    
    <type_specifier> ::= <integer_type> | <typedef_name>
    
    <typedef_name> ::= <identifier>
```

## Semantic rule: an `<identifier>` is known as a `<typedef_name>` only if it is 
   currently known as a typedef in the active scope.

## Requirements
  - Add `typedef` as a supported storage-class specifier
  - Support simple aliases for existing integer types
    - typedef char C;
    - typedef short S;
    - typedef int I;
    - typedef long L;
  - typedef declarations create type aliases not variables
  - typedef names may be used in:
    - object declarations
    - function return types
    - function parameter types
    - casts
    - `sizeof(type-name)`
  - Reject typedef declarations with initializers
  - Reject duplicate typedef names in the same scope
  - Reject use of unknown identifiers as type specifiers

## Valid Tests
```C
    typedef int I;
    
    int main() {
        I x = 3;
        return x;   // expect 3
    }
```

```C
    tydedef char C;
    
    int main() {
        C x = 65;
        return x;    // expect 65
    }
```

```C
    typedef long L;
    
    L f(L x) {
        return x + 1;  
    }
     
     main() {
        return f(4);   // expect 5
     }
```

```C
    typedef int I;
    
    int main() {
        return sizeof(I);   expect 4
    {
```

## Invalid tests
typedef declarations do not have initializers
```C
    typedef int I = 3;         // invalid
```

duplication typedef declarations
```C
   typedef int I;
   typedef int I;     // INVALILD
```

type is never defined
```C
    I x;      // INVALID
    
    int main() {
        return 0;
    {
```

no valid base type
```C
    typedef I;   // INVALID
```

## Out of scope
  - pointer typedefs
  - array typedefs
  - function pointer typedefs
  - struct typedefs
  - enum typedefs
  - 



