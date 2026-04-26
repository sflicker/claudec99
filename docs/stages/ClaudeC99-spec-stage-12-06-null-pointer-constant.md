# ClaudeC99 Stage 12.06

## Task
  - add support for null pointer constant (0)
  - pointer equality and inequality comparisons

## Definitions

### Null Pointer Constant
  - The literal `0` may be used as a null pointer constant
  - It may be:
    - assigned to a pointer
    - returned from a pointer function
    - compared with a pointer
**Examples**
    ```C 
        int *p = 0;
        if (p == 0) {...}
    ```
    
### Pointer Equality
  - Supported operations
  - `==`, `!-`
  - valid comparisons:
    - pointer vs pointer (same type)
    - pointer vs null(0)

## Grammar Changes
**NONE**

## Requirements
  - Allow `0` to be assigned to any pointer type:
    - Example: `int *p = 0;`
  - Allow `0` to be returned from pointer-returning functions
    - Example: `int *f() { return 0; }`
  - Treat `0` as a special case when type-checking against pointer types.

## Pointer Comparisons
  - Support 
    ```C
        p == q
        p != q
        p == 0
        p != 0
    ```
    - Type Rules:
      - ` pointer T == Pointer T` -> ok 
      - ` pointer T == 0` -> ok
      - ` pointer T == Pointer U` -> ERROR (if T != U)
      - ` pointer T vs non-zero interger` -> ERROR

## Result Type
  - All comparisons return `int` (0 or 1, consistent with existing behavior

## Codegen Notes
  - Represent null pointer as
    - `mov rax, 0`
  - Pointer comparison
    - For `p == q`
    - Generate 
      ```asm
          cmp rax, rbx
          sete al
          movzx rax, al
      ```
    - For `p != q`
    - Generate
      ```asm
          cmp rax, rbx
          setne al
          movzx rax, al
      ```
  - Same logic applies for comparisons with `0`

## Semantic Rules
  - `0` is treated as:
    - `int` in integer contexts
    - `pointer-to-T` in pointer contexts
  - This is the **only implicit pointer conversion allowed** in this stage

## Tests to generate
  - Null assignment
  - Pointer equality (same address)
  - Pointer inequality
  - Compare pointer with null
  - Pointer return null
  - Invalid: pointer vs non-zero integer
  - Invalid: mismatched pointer types
  - 
  - Other Appropriate Tests

## Out of Scope
  - Pointer arithmetic
  - Relational pointer comparisons
  - Arrays
  - Strings
  - Function Pointers
  - General pointer casting (except 0)
  - Full implicit comparisons

## Done Criteria
  - `0` works as null pointer constant in pointer contexts
  - Pointer equality and inequality comparisons work
  - Type checking correctly rejects invalid comparisons
  - All previous tests pass
  - All new tests pass


  