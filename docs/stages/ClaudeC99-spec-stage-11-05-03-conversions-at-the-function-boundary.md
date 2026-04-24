# Claude C99 Stage 11-05-03 

## Task:
  - Extend semantic analysis and code generation to support argument-to-parameter conversions
    and return-value conversions for typed functions.

## Goals:
  - Apply type conversions at function call boundaries
  - Apply type conversions at return statements
  - Use existing type system rules from Stage 11.2-11.2
  - Ensure correct truncation and widening behavior
  - Keep implementation consistent with existing calling convention.
  - Preserve all existing valid behavior and tests


## Overview
  In stage 11.5-02, function calls became **typed expressions** based on function signatures.

  This stage completes the function-type system by implementing:
  - **Argument conversion** at call sites
  - **Parameter storage using declared typse**
  - **Return-value conversion at `return` statements

  After this stage:
  - Types are represented across function boundaries
  - Narrowing and widening behave correctly
  - Function calls behave consistently with local assignments

## Requirements
  1. Argument=-to-parameter conversion
     for a function call:
     ``` f(arg1, arg2, arg3) ```
     for each argument `arg_i`:
     1. Evaluate `arg_i` using existing expression rules
     2. Determine parameter type `T_i`
     3. Convert the argument value to type `T_i`
  
     **Conversion rules**
       - if source size < destination size -> **widen (sign-extend)**
       - if source size > destination size -> **truncate**
       - if same size -> no change

     Examples
     ```
        char f(char x);
        long a = 258;
     
        f(a);    // argument truncated to char

     ```
     
     ```C
         long f(long x);
     
         char a = 5;
         f(a);   // argument widen to long
     ```
  2. Parameter storage at function entry
     When parameters are stored in local stack slots:
       - Allocate storage based on parameter type size and alignment
       - Store parameter values using correct width
     Examples:
         - `char` -> 1 byte
         - `short` -> 2 bytes
         - `int` -> 4 bytes
         - `long` -> 8 bytes

  3. Return-value conversion
     For a return statement:
     ` return expr; `
 
     The compiler must:
     1. Evaluate `expr`
     2. Convert result to function's declared return type
     3. Place converted value in return register
     
     **Conversion rules**
       - Same as assignment:
         - widen -> sign-extend
         - narrow -> truncate

     **Examples**
     ``` 
        char f() {
           long x = 258;
           return x;    // truncated
        }
     ```
     ```
        long f(x) {
           char x = 5;
           return 5;   // widen (sign-extend)
        }
     ```

  4. Call expression typing (unchanged)
    - The type of a function call expression remains:
       The declared return type of the function
    - This behavior was implemented in Stage 11.5-02

  5. Integeration with existing type system
  This stage must reuse existing mechanisms:
    - Type sizes and allignment (Stage 11.2)
    - Load/Store width rules (Stage 11.2)
    - Integer Promotions (Stage 11.3)
    - Expression typing (Stage 11.3-11.4)

## Grammar
No Grammar changes are required.

## In Scope:
  - Argument conversion at call sites 
  - Parameter storage using declared types
  - Return-value conversion
  - Typed interaction across function boundaries

## Out of Scope:
  - Strict type errors (still allow, just convert)
  - Function signature compatibility accuracy
  - ABI-level calling accuracy
  - Floating point types
  - Unsigned
  - Casts
  - Variadic functions

## Semantic Behavior
### Argument narrowing
``` 
  char f(char x) { return x; }
  
  int main() {
     long a = 257;
     return f(a);                  // expected result: 1
  }
```

### Argument widening
``` 
    long f(long x) { return x; }
    
    int main() {
        char a = 5;
        return f(a);        // expected result: 5
```

### Return widening
``` 
    long f() {
        char x = 6;
        return x;
    }
    
    int main() {
        return f();         // expected result: 6
    }
```

### Mixed parameter types
``` 
    short add(short a, char b) {
        return a + b;
    }
    
    int main() {
        short x = 10;
        char y = 3;
        return add(x, y);    // expected result: 13
```


## Codegen expectations
1. Argument handling
  - Convert arguments before placing into registers or stack
  - Use correct register width or memory width

2. Parameter storage
  - Store parameters in stack frame using correct size and alignment

3. Return values
  - ensure final return register value reflects converted type

## Data Flow Summary
1. argument expression
2. evaluate expression
3. convert to parameter type
4. pass/store
5. function body
6. return expression
7. convert to return type
8. return register

## Testing
### Existing tests
  - All existing test must pass unchanged

### New Tests
** Add Tests to Cover **
  - argument narrowing
  - argument widening
  - return narrowing
  - return widening
  - mixed parameter types
  - interaction with arithmetic expression

## Completion Criteria
**This stage is complete when:**

  - Argument values are converted to parameter types at call sites
  - Parameters are stored using declared type size and alignment
  - Return values are converted to function return type
  - Function calls behave consistently with local assignments
  - No regressions occur
  - All tests pass

## Summary

This stage completes the integration of the integer type system across function boundaries.

After completion:

  - Types are respected during function calls
  - Types are respected during returns
  - Narrowing and widening behave consistently everywhere
  - The compiler’s type system is now coherent across:
    - local variables
    - expressions
    - control flow
    -function interfaces

