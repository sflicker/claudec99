# ClaudeC99 stage 11.5-2

## Task:
- Extend semantic analysis to support type function signatures and typed function call expressions.

## Goals
  - Use the typed function declarations introduced in Stage 11.5-1
  - Add semantic understanding of function signatures (return type and parameter types)
  - Ensure function call expressions are assigned the correct type based on the callee's declared return type
  - Keep codegen unchanged
  - Preserve all existing valid behavior and tests

## Overview
In Stage 11.5-1, the grammar and parser were extended so functions and parameters can carry integer types.
This stage builds on that by introducing semantic awareness of function signatures:
  - A function is now represented by a signature:
    - return type
    - ordered list of parameter types
  - Function calls are analyzed using this signature
  - The type of a function call expression is the function's declared return type.
This stage does not yet perform argument type conversions or return-value conversions. Those will be handled in a later stage.

## Requirements:
1. Function symbols must store a full signatures;
  - Function name
  - Return Type
  - parameter count
  - ordered list of parameter types
Example:
  ``` long sum(char a, char b); ```
  - Must produce a signature equivalent to:
  sum : (char, short) -> long

2. Populate function signatures:
   When processing a function declaration or definition:
     - extract the declared return type
     - extract parameter types in order
     - store this information in the function symbol
   Both forward declarations and full definitions must populate consistent signatures.

3. Function call resolution
   When analyzing a function call
    ``` sum(x, y) ```
   The compiler must:
     - resolve the `sum` to a function symbol
     - retrieve the stored signature
     - enforce that the number of arguments matches the parameter count
   Argument count mismatches must continue to produce an error.

4. Argument expression typing
   Each argument expression
     - is analyzed using existing expression typing rules (stages 11.3 and 11.4)
     - retains its own computed type
   No conversion to parameter types is performed in this stage.

5. Function call expression type
   The type of a function call expression must be:
     The declared return type of the call function.

   example:
   give function `long f();` then the expression `f()` would have type of `long`.

6. Integration with expression typing
   Function call expressions must integrate with existing type rules:
     - promotions
     - common integer type selection
     - arithmetic expressions
     - comparisons
     - logical expressions
   Example 
   ```
   long f();
   
   int main() {
       char x = 2;
       return x + f();
   }

   ```
   
   `x` promotes to `int`
   `f()` has type `long`
    common type is `long`

7. AST updates (if needed)
    - Function call AST nodes may store:
      - resolved function symbol
      - or resolved resolved return type
    - This is optional if the type can be retrieved through symbol lookup

## Grammar updates
    - NONE

## In Scope
  - Function symbol signature representation
  - return type propagation to call expressions
  - parameter type storage in function symbols
  - argument count validation
  - integration with existing type rules

## Out of Scope
  - Argument to parameter type conversions
  - return type conversions
  - code generation changes
  - strict parameter type checking
  - function signature compatibility checks between declarations and definitions
  - ABI or calling convention changes
  - Diagonostics for type mismatches beyond arity errors

## Semantic Behavior
  - Typed call expression
    ```
       long f() { return 5; }
     
       int main() {
          return f();      // f() has type long
       }
    ```

  - Call participates in arithmetic
    ```
        long f() { return 5; }
      
        int main() {
            char x = 2;
            return x + f();     // f() has type long, common type is long.
    ```
    
  - Parameter types stored but not enforced
  ```
    short g(char a, char b);
    
    int main() {
        int x = 1;
        int y = 2;
        return g(x, y);    // allowed in this stage. argument count match. no type compatibility enforcement in this stage.
    }
  ```

  - Arity check remains enforced
  ```
    int f(int a, int b);
    
    int main() {
        return f(1);     // ERROR: incorrect number of arguments
    }

  ```

## Testing
### Existing Tests
   - all existing and invalid tests must continue to pass

### New Tests
   - Add tests to verify:
     - function signatures store correct return and parameter types
     - function calls receive correct expression type
     - arithmetic with function calls uses the correct resulting type
     - argument counts errors still occur

## Completion Criteria
This stage is complete when:
  - Function symbols store full typed signatures
  - function call expressions are assigned the declared return type
  - argument count validation works as before
  - no codegen changes were introduced
  - all existing tests pass without regression
  - new tests confirm correct typing behavior for function calls

## Summary
This stage introduces semantic awareness of function signatures

After completion:

  - Functions are no longer treated as int-only constructs
  - Function calls become fully typed expressions
  - The type system now flows correctly across function boundaries

This prepares the compiler for the next stage:
   - argument to parameter conversions
   - return-value conversions
   - typed codegen for function calls and returns


