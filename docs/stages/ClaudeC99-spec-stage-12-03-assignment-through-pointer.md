# ClaudeC99 Stage 12.3

## Task
  - Extend the compiler to support writing through pointers using the dereference operator.

## Core Test
``` 
    int main() {
      int x = 1;
      int *p = &x;   // pointer holds address of x
      *p = 9;        // assign through pointer value 9 (x is updated)
      return x;      // expect 9
```

## Definitions
  - The expression ` *p = value; `
  - Stores `value` at the memory location pointed to by `p`.

## Grammar updates
  - **None**

## Semantic Rules
**Valid Assignment Targets (lvalues)**
Assignment left-hand side must be one of:
  - Variable reference:  `x = 5;`
  - Dereference expression: `*p = 5;`

**Dereference as Lvalue**
  - `*expr` is an lvalue if:
    - `expr` has a pointer type

**Type Rules**
  - For: `*p = value; `
    - `p` must be of type `pointer to T`
    - `value` must be assignment to type `T`

**Invalid Cases**
  ``` 
      *p = value;   // error if p is not a pointer
      (x + 1) = 5;  // error not an lvalue
  ```

## AST Notes
No new node types required.

Reuse:
  - `AST_DEREF` as an lvalue
  - `AST_ASSIGN` (or equivalent)

**Important**
Ensure AST Supports:
``` 
    ASSIGN
       left: DEREF(p)
       right: expr
```

## Codegen Notes
  Assembly logic is similar to:
  - Existing Assignment to variable  
    - ` mov [rbp - offset], value` 
  - New Assignment through pointer
    ``` 
      ; for *p = value;
      ; evalulate p -> address in rax`
      ; evaluate value -> result in rbx (or stack)
      mov[rax], rbx
    ```
    
### Size Handling
  Store must match the base type:
  Type          Store
  char          byte
  short         word
  int           dword
  long          qword
  Examples:
  ```asm 
     mov byte [rax], bl
     mov word [rax], bx
     mov dword [rax], ebx
     mov qword [rax], rbx
  ```

### Important Distinction
  - `*p` in rvalue context -> load
  - `*p` in lvalue context (assignment target) -> do *NOT* load, use as address

### Implementation Notes
**Key Change**
In assignment codegen:
  - Detect if LHS is:
    - `AST_VAR_REF` -> existing behavior
    - `AST_DEREF` -> new pointer store path
**Recommended helper**
    - bool is_lvalue(ASTNode * node);
Return true for:
    - `AST_VAR_REF`
    - `AST_DEREF`

## Tests
### Basic write through pointer
```C 
    int main() {
        int x = 1;
        int *p = &x;
        *p = 9;
        return x;    // expect 9
    }
```
### Write than read
```C
    int main() {
        int x = 3;
        int *p = &x;
        *p = 5;
        return *p;    // expect 5
    }
```

### Nested dereference write
```C
    int main() {
        int x = 2;
        int *p = &x;
        int **pp = &p;
        **p = 8;
        return x;     // expect 8
```

### Multiple writes
```C 
    int main() {
        int x = 1;
        int *p = &x;
        *p = 2;
        *p = 3;
        return 3;    // expect 3
    }
```

### Invalid: dereference non-pointer
```C 
    int main() {
        int x = 5;
        *x = 3;       // ERROR dereference non-pointer
        return x; 
    }
```

### Invalid: non-lvalue assignment
```C
    int main() {
        int x = 5;
        (x + 1) = 3;   //ERROR non l-value assignment
        return x;
    }
```

### Done Criteria
  - `*p = value` compiles and runs correctly
  - Dereference works as both:
    - rvalue (previous)
    - lvalue (this stage)
  - Codegen correctly emits memory stores via pointer
  - Type rules enforced
  - All previous tests pass
  - New tests pass

### Out of Scope
  - Pointer arithmetic
  - Pointer comparisons
  - Arrays
  - Structs
  - Complex lvalue expressions beyond `*expr` and variables
