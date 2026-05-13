# ClaudeC99 stage 38 - void keyword pointer

## Goals
  - Add `void` as a key specifier
  - support functions returning void
  - support bare `return;` statements in void functions
  - Support base `(void)` parameter lists to mean no parameters
  - Support `void *` as a generic object pointer type
  - Allow implicit assignment/conversion between `void *` and other pointer types

## Tokens
  - add keyword token for `void`
 
## Grammar Updates
```ebnf
  <type_specifier> ::= "void"
                      | <integer_type>
                      | <typedef_name>
                      | <enum_specifier>
                      | <struct_specifier>
  
  <return_statement> ::= "return" [ <expression> ] ";"

  <parameter_list> ::= "void" 
                    | <parameter_declarator> { "," <parameter_declarator> }
```
**Note**  `int f(void)` means function f takes zero parameters.

## Requirements
  1. `void` functions
    support
  ```C
     int global;

     void set_global(void) {
         global = 42;
     } 
     
     int main() {
         set_global();
         return global;     // expected 42
     }
  ```
  2. bare `return;`
  ```C
      void set_value(int *p) {
          *p = 42;
          return;
      } 
  ```
   Also allow falling off the end of a `void` function
   ```C
       void set_value(int *p) {
           *p = 42;
       }
   ```
   
3. Reject return a value from a `void` function
   ```C
       void bad(void) {
          return 1;      // INVALID
       }
   ```
   
4. Reject empty return from non-void function
    ```C
       int bad(void) {
           return;
       }
    ```
5. `void` cannot be an object type
    ```C
        int main() {
            void x;         // INVALID
            return 0;
        }
    ```
    Reject `void` objects at global scope as well
    ```C
       void global;         // INVALID 
   ```

   Reject `sizeof(void)`

6. `void *` is allowed
   Support
   ```C
       int main() {
           int x;
           int *p;
           void *v;
      
           x = 42;
           p = &x;
           v = p;
           p = v;
   
           return *p;     // expect 42
       }
   ```
   
7. Allow object pointer to `void *`
   These should be accepted
   ```C
       int *ip;
       char *cp;
       struct Point *pp;
       void *vp;
   
       vp = ip;
       vp = cp;
       vp = pp;
   ```
   
8. Allow `void *` to object pointer
    These should be accepted
    ```C
       int *ip;
       void *vp;
   
       ip = vp;
    ```
    implicit conversion to `void *` is allowed
9. Reject dereferencing `void *`
   ```C
       int main() {
           void *p;
           return *p;   // INVALID
       }
   ```
   Also Reject
   ```C
        void *p;
        p[0];    //INVALID
   ```
   
10. Reject pointer arithmetic on `void *`
    Reject
    ```C
        void *p;
        p = p + 1;
    ```
    
## Tests
### Valid Tests
Basic void function
```C
    int global;
    
    void set_global(void) {
        global = 42;
    }
    
    int main() {
        set_global();
        return global;    // expect 42
    }
```
Void function with pointer mutation
```C
    void set_value(int *p) {
        *p = 37;
    }
    
    int main() {
        int x;
        set_value(&x);
        return x;        // expect 77
    }
```
Bare return from void function
```C
    void set_value(int *p) {
        *p = 12;
        return;
    }
    
    int main() {
        int x;
        set_value(&x);
        return x;   // expect 12
    }
```
`int f(void)` means no arguments
```C
    int get_value(void) {
        return 35;
    }
    
    int main() {
        return get_value();    // expect 35
    }
```

`void *` assignment from `int *`
```C
    int main() {
        int x;
        int *p;
        void *v;
        
        x = 42;
        p = &x;
        v = p;
        p = v;
        
        return *p;          // expect 42
```

`void *` with struct pointer
```C
    struct Point {
        int x;
        int y;
    }
    
    int main() {
        struct Point *p;
        sturct Point *pp;
        void *vp;
        
        p.x = 10;
        p.y = 20;
        
        pp = &p
        vp = pp;
        pp = vp;
        
        return pp->x + pp->y;   // expect 30
```

### Invalid Tests
Cannot declare object of type void
```C
    int main() {
        void x;    // INVALID
        return 0;
    }
```

Cannot return value from void function
```C
    void bad(void) {
        return 1;     // INVALID
    }
    
    int main() {
        bad();
        return 0;
    }
```

Cannot use empty return in int function
```C
    int bad(int);
        return;     // INVALID
    }
    
    int main() {
        bad(1);
        return 0;
    }
```

Cannot dereference a void pointer
```C
    int main() {
        void *p;
        return *p;    //INVALID
    }
```

Cannot do pointer arithmetic on void pointer
```C
    int main() {
        void *p;
        p = p + 1;    // INVALID
        return 0;
    }
```

Cannot use void functions result as value
```C
    void f(void) {
        return;
    }
    
    int main() {
        return f();    // INVALID
    }
```

ALSO
```C
    void f(void) {
        return;
    }
    
    int main() {
        int x;
        x = f();     // INVALID
        return x;
    }
```

## Codegen

`void` return type
a `void` function does not need to place a meaningful value in `rax`
example for c void function
```C
    void f(void) {
        return;
    }
```
Generate something like a normal function epilogue
```asm
    mov rsp, rbp       
    pop rbp
    ret
```

Calling a `void` function
```C
    f();
```
Generate something like
```asm
    call f
```

## Semantic Notes
  - if the AST expression type is `void` the the semantic checker should reject using
    it where a value is required.
  - Allow conversion object pointer -> void *
  - allow conversion void * -> object pointer
  - reject dereference of pointer to void
  - reject arithmetic on pointer to void

## Out of scope
  - const void *
  - volatile
  - sizeof(void)
  - pointer arithmetic on void *
  - function pointers returning void, unless already naturally supported
  - functions with void parameters mixed with other parameters, e.g. int f(void, int x)
  - old-style c distinction between f() and f(void)
