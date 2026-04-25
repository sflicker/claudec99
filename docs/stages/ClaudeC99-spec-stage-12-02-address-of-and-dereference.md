# Claude C99 Stage 12.2

## Task
- Add unary address-of `&expr` and dereference `*expr`, enough to support reading through a pointer.

## Core Test
```
  int main() {
    int x = 7;
    int *p = &x;
    return *p;      // expect 7
  }
```

## Token update
- add token for `&` (ampersand)

## Grammar update
``` 
<unary_expression> ::= <unary_operator> <unary_expression>
                      | <postfix_expression>
                      
<unary_operator> ::= "+" | "-" | "!" | "++" | "--" | "*" | "&"                   

```

## Semantic Rules (Important)
### Address-of Operator `&`
  - Operand must be an lvalue (e.g., variable)
  - Result type is:
  ``` T -> pointer to T ```
  - Example:
  ``` 
     int x;
     int *p = &x;
  ```
  - Invalid:
  ``` 
     &(x + 1)  // not an lvalue -> error
  ```

### Dereference Operator `*`
  - Operand must be a **pointer type**.
  - Result type is:
  ``` pointer to T -> T ```
  - Result is an **lvalue**
  - Example
  ``` 
     int *p;
     int x = *p;
  ```

### AST Updates
Add new node kinds:
  - AST_ADDR_OF
  - AST_DEREF

Example shapes:
``` 
  // &x
  AST_ADDR_OR
     child: x
     
  // *p
  AST_DEREF
     child: p
```
Type propagation:
  - &expr -> pointer(expr.type)
  - *expr -> expr.type->base

### Type Checking Rules
  - `&expr`
    - Requires: `expr` is lvalue
    - Result: pointer type
  - `*expr`
    - Requires: `expr.type == TYPE_POINTER`
    - Result: base type

### Codegen Notes (x86-64 mindset)

**Address-of** `&x`
  - Do **not** load value of `x`
  - Instead compute its address
Example:
```asm 
   lea rax, [rbx - offset]   ; address of local variable
```

**Dereference** `*p`
  - Evaluate `p` -> address in register
  - Load value from that address
Example:
```asm
   mov rax, [rax]   ; load value from pointer
```
  - Use correct size:
    - `char` -> byte
    - `short` -> word
    - `int` -> dword
    - `long` -> qword

### In Scope
  - `&variable`
  - `*pointer`
  - pointer assignment
    - ` p = &x;`
  - reading through pointer
    - ` return *p;`

### Out of Scope (this stage)
  - ` *p = value; `
  - ` &*p edge cases`
  - pointer arithmetic
  - pointer comparisons

## Additional Tests
Basic address-of + dereference
``` 
   int main() {
      int x = 5;
      int *p = &x;
      return *p;      //expect 5
```

Assignment through pointer variable
``` 
    int main() {
        int x = 3;
        int *p;
        p = &x;
        return *p;    // expect 3
    }
```

Nested dereference
``` 
    int main() {
        int x = 9;
        int *p = &x;
        int **pp = &p
        return **p;    // expect 9
```

**Invalid**: dreference non-pointer
``` 
    int main() {
        int x = 5;
        return *x;     // error
```

**Invalid**: address of non lvalue
``` 
    int main() {
        int x = 5;
        return &(x + 1);   // error
    }
```

## Done criteria
  - `&` and `*` parse correctly
  - Type checking enforced:
    - `&` requires lvalue
    - `*` requires pointer
  - Codegen correctly:
    - Produces address with `lea`
    - Loads through pointer with `mov`
  - All previous tests pass
  - New pointer tests pass

