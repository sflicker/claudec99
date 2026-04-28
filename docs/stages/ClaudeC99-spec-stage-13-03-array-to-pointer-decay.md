# ClaudeC99 Stage 13.4

## Task
  - Add array-to-pointer decay in expression contexts
  - Allow local arrays to be passed to pointer parameters
  - Allow local array to initialize compatible pointer variables

## Requirements

### Array-to-pointer decay

An expression of array type `T[N]` should decay to `T *` in ordinary value contexts.
Examples:
  ```C
      int main() {
        int a[2];
        int *p = a;
        return 0;
  ```

  ```C
      int first(int *p) {
        return p[0];
      }
      
      int main() {
        int a[2];
        a[0] = 42;
        return first(a);    // expect 42
  ```

### Type Rules
Given:   `T a[N];`   
The expression:
`a`
Has array type when used as an object/lvalue, but decays to:
`T *`
when used as a value expression.

### Codegen rules
For a local array variable used in a decayed expression, generate the address of the first element.
So `int *p = a;`  Should behave like `int *p = &a[0];`
The generatec value is the array base address, not a load from the stack slot.

### Do not decay in these context
  - For this stage, array decay should not make whole-array assignment legal:
  ```C 
      int a[2];
      int b[2];
      a = b;   // ERROR 
  ```
  - Also reject assigning directly to an array object:
  ```C
      int a[2];
      a = 0;  // ERROR 
  ```

### Out of scope
  - pointer arithmetic
  - pointer difference
  - Multidimensional arrays
  - Array parameters declared as `int a[]`
  - sizeof
  - String literals
  - Array initializers

### Tests
Pointer initialization from array
```C
    int main() {
        int a[2];
        int *p = a;
        a[0] = 37;
        return *p;    // expect 37
    }
```

Pass array to pointer parameter
```C
    int first(int *p) {
        return p[0];
    }
    
    int main() {
        int a[2];
        a[0] = 42;
        return first(a);   // expect 42
    }
```

Char array decay
```C
   int first(char *p) {
       return p[0];
   }
   
   int main() {
       char a[2];
       a[0] = 12;
       return first(a);   // expect 12
   }
```

Long array decay
```C
    long first(long *p) {
        return p[0];
    }
    
    int main() {
        long a[2];
        a[0] = 42;
        return first(a);   expect 42
    }
```

Invalid: whole-array assignment
```C
    int main() {
        int a[2];
        int b[2];
        a = b;  // ERROR
        return 0;
    }
```

Invalid: assign scalar to array
```C
    int main() {
        int a[2];
        a = 0; // ERROR
        return 0;
    }
```

### Implementation Notes
  - keep array type distinct from pointer type:
  - add a helper such as:
    ` Type * decay_array_type(Type *type); `
  - or
  - ` Type * expression_value_type(ASTNode *expr); `
  - For codegen, variable references need special handling
    - scalar local variable: load value from stack slot
    - array local variable in decayed context: product address of array base
    - array element expression: existing indexing behavior still produces element lvalue/value as needed.

