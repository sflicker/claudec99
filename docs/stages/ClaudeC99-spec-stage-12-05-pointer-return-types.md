# ClaudeC99 Stage 12.5

## Task
  - Extend the Compiler to Support Pointer Return Types  

## Grammar Updates
```ebnf
    Modified:
    <function> ::= <type> <identifier> "(" [ <parameter_list> ] ")" ( <block_statement> | ";" )
    
    Reuse from previous stage:
    <type> ::= <integer_type> { "*" }
```

## Requirements
  - Support pointer types as function return values
  - Preserve existing integer return support
  - Return pointer values through `rax`
  - Allow assigning pointer return values to pointer locals
  - Type-check return expressions against function return type
  - Function declarations and definitions must support pointer return types
  - Pointer return types **must exactly match** (same base and level of indirection)
  - No implicit conversions between pointer types allowed.

## Return type rules
  for 
```C
    int *f(...) {
        return expr;
    }
```
  - `expr` must be of type `int *`
  - Exact type match required (no implicit pointer conversions in this Stage)

## Out of scope (for this stage)
  - pointer arithmetic
  - null
  - return pointer literal 0
  - arrays
  - strings
  - function pointers
  - lifetime checks

## Tests
### Valid: Return pointer
```C 
    int *identify(int * p) {
        return p;
    }
    
    int main() {
        int x = 7;
        int *p = identify(&x);
        return *p;       //expect 7
    }
```

### Valid: Nested Pointer Return
```C 
    int **pd2(int **p) {
        return pp;
    }
    
    int main() {
        int x = 5;
        int *p = &x;
        int **pp = &p;
        return **id2(pp);      // expected 5
    }
```

### Invalid: return incorrect pointer type
```C
    int * identity(char * p) {
        return p;      // Compiler ERROR: returning incorrect pointer type
    }
    
    int main() {
        char a = 1;
        return(&a);
    }
```

### Invalid: returning non pointer
```C
    int * identity(int *p) {
        return 1;    // Compiler ERROR: returning non pointer
    }
    
    int main() {
        int a = 1;
        return(&a);
    }

```

### Invalid: Assigning pointer return to incorrect pointer type
```C
   int * identity(int * p) {
       return p;
   }
   
   int main() {
       int a = 1;
       char * p = identity(&a);    // compiler ERROR: assigning pointer return to incorrect pointer type
   }    
```

### Invalid: Assigning pointer return to non pointer 
```C
    int * identity(int *p) {
        return p;
    }
    
    int main() {
        int a = 1;
        int b = identity(&a);    // Compiler ERROR: assigning pointer return to non pointer
    }
```

### Invalid: Return Pointer Literal 0
```C 
    int *f() {
        return 0;    // should be ERROR for this stage
    }
    
    int main() {
        char a = 1;
        int * p = f();
        return 0;
    }
```

### Invalid: Return pointer of incorrect level of indirection
```C 
    int **f(int *p) {
        return p;       // ERROR: incorrect level of indirection
    }
    
    int main() {
        int a = 1;
        return f(&a);
    }
```