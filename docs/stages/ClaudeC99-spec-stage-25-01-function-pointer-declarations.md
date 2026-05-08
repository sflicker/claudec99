# ClaudeC99 Stage 25-01 function pointer declarations

## Goal
  - Add support for declaraing variables, parameters and file-scope objects
    whose type is "pointer to function".
    Examples
   ```C
        int (*fp)(int);
        int (op)(int a, int b);
        static int (*handler)(int);
        extern int (*callback)(int);
   ```
  - **DO NOT** support assigning functions to those pointers and calling through
    them in this stage.

## Main Feature
Given:
    ```C
        int (*fp)(int);
    ```
    `fp` is a pointer a function taking `int` and returning `int`.
Also support multiple parameters
    ```C
        int (*fp)(int a, int b);
    ```

And pointer types
    ```C
        int * (*fp)(int *);
    ```
Function pointers should be accepted instead of rejected.

## Requirements
  1. Location function pointer variables
  ```C
      int main() {
          int (*fp)(int);
          return 0;
      } 
  ```
  The local variable `fp` should be allocated like an ordinary pointer-sized local.
  No assignment or call in this stage.
  
  2. File-scope function pointer objects are allowed
  ```C
      int (*fp)(int);
      
      int main() {
          return 0;
      }
  ```
    this should be allocated similar to other file scope pointers.
  3. static function pointer objects are allowed 
  ```C
      static int (*fp)(int);
      
      int main() {
          return 0;
      } 
  ```
    this should behave like a static file-scope object with internal linkage

  4. Extern function pointer declarations are allowed
  ```C
      extern int (*fp)(int);
      
      int main() {
          return 0;
      } 
  ```
   This should declare `fp` as an external object of function-pointer type
     and should not emit storage.

   5. Function pointer parameters are allowed
   ```C
       int apply(int (*fp)(int), int x) {
          return 0;
       } 
   ```
      
## Type Compatibility
  Function pointer types should include
  ```
    return type
    parameter count
    parameter types
  ```
  These are the same type
  ```C
      int (*a)(int);
      int (*b)(int);
  ```

  These are different
  ```C
      int (*a)(int);
      int (*b)(long);
  ```
  These are also different
  ```C
      int (*a)(int);
      long (*b)(int);
  ```

## Valid Tests
Local function pointer declaration
```C
    int main() {
        int (*fp)(int);
        return 0;   // expect 0
    }
```

Local function with two parameters
```C
    int main() {
        int (*fp)(int a, int b);
        return 0;   // expect 0
    }
```

File scope function pointer
```C
    int (*fp)(int);
    
    int main() {
        return 0;   //expect 0
    }
```

static file-scope function pointer
```C
    static int (*fp)(int);
    
    int main() {
        return 0;  // expect 0
    }
```

extern function pointer followed by definition
```C
    extern int (*fp)(int);
    int (*fp)(int);
    
    int main() {
        return 0;    //expect 0;
    }
```

function pointer parameter
```C
    int apply(int (*fp)(int), int x);
    
    int main() {
        return 0;   // expect 0;
    }
```

Function pointer returning pointer
```C
    int *(*fp)(int *);
    
    return main() {
        return 0;   // expect 0
    }
```

## Invalid Tests
Function pointer declaration compatibility mismatch
```C
    extern int (*fp)(int);
    int (*fp)(long);      // INVALID
    
    int main() {
        return 0;
    }
```

Function pointer return type mismatch
```C
    extern int (*fp)(int);
    long (*fp)(int);    // INVALID
    
    int main() {
        return 0;
    }
```

Duplicate static function pointer definition
```C
    static int (*fp)(int);
    static int (*fp)(int);   // invalid
    
    int main() {
        return 0;
    }
```

Calling through function pointer not supported at this stage
```C
    int main() {
        int (*fp)(int);
        return fp(3);   // not supported at this stage
    }
```

Assigning function to function pointer not supported at this stage
```C
    int f(int x) {
        return x;
    }
    
    int main() {
        int (*fp)(int);
        fp = f;            // not supported at this stage.
        return 0;
    }
```

## Out of Scope
  ```C
      fp = f;                  // assigning function names to function pointers
      return fp(3);            // indirect calls
      return (*fp)(3)          // explicit dereference indirect calls
      int (*factory())(int);  //  function returning pointer to function
      int (**fp)(int);         // pointer to function pointer 
  ```

## Implementation notes
The important distinct is
```C
   int f(int);
```
declares a function

```C
    int (*fp)(int);
```
declares a pointer to a function

Type builder needs to distinguish
```
    Function Type:
        Function(int) returning int
        
    Pointer-to-function type:
        pointer to function(int) returning int

```