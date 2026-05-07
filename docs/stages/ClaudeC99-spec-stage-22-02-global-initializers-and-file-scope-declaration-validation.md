# ClaudeC99 Stage 22-02 Global Initializers and file scope declaration validation

## Task
  - Add support for simple initialized file-scope object validation
  - Tighten validation around file scope declarations

Support scalar global initializers
```C
   int x = 3;
   char = 65;
   short s = 123;
   long l = 100000L;
   
   int a = 1, b, c = 3;
```

Requirements:
  - Allow file-scope object declarations with constant integer expressions
   ```C
      int x = 1;
      char c = 65;
      short s = 1000;
      long l = 123456L; 
  ```
    A file scope object initializer must be a compile time constant expression
  - Emit initialized globals in .data
   ```C
      int x = 3;
      char c = 54;
      short s = 123;
      long l = 1000000L 
   ```
   NASM style output (initialized data in the .data section)
   ```nasm
       section .data
       x: dd 3
       c: db 65
       s: dw 123
       l: dq 100000
   ```
   - Preserve uninitialized globals in .bss
   ```C
       int x;
       long y; 
   ```
   ```asm
       section .bss
       x: resd 1
       y: resq 1
   ```
   - Reject non-constant global initializers
   ```C
      int f() {
         return 1;
      }
      
      int x = f();     // INVALID
   ```
   - Reject reference to variables
   ```C
      int a = 1; 
      int b = a;     // INVALID  
   ```
   - Reject function initializers
   - Reject duplicate global objects
   ```C
       int x;
       int x;      // INVALID 
   ```
   - Reject incompatible redecorations
   ```C
       int x;
       long x;   // INVALID 
   ```
   - Reject function object name conflicts
     Functions and file scope object s share the ordinary identifier namespace
   ```C
      int f;
      
      int f() {    // INVALID
         return 1;
      } 
   ```
   ```C
       int f;
       int f();    // INVALID
   ```

## Tests
Basic initialized global
```C
    int x = 7;
    
    int main() {
        return x;   // expect 7
    }        
```

Initialized globals of different types
```C
    char c = 1;
    short s = 2; 
    int i = 3;
    long l = 4;
    
    int main() {
        return c+s+i+;   // expect 10
    }
```

initialized and initialized globals together
```C
    int a = 5;
    int b;
    
    int main() {
        b = 7;
        return a + b;    // expect 12
    }
```

Multiple declarators
```C
    int a = 1, b, c = 3;
    
    int main() {
        b = 2;
        return a + b + c;    // return 6;
    }
```

Global persists and initial value 
```C
    int counter = 10;
    
    int inc() {
        counter = counter + 1;
        return counter;
    }
    
    int main() {
        inc();
        inc();
        return counter;    // expect 12
    }
```

Local Shadows global

```C
    int x = 10;
    
    int main() {
        int x = 4;
        return x;     // expect 4
    }
```

## Invalid Tests
Function Initializer
```C
     int f() = 3;   // INVALID
```

non-constant initializer: function
```C
    int f() {
        return 1;
    }
    
    int x = f();   //INVALID
    
    int main() {
        return x;
    }    
```

Non constant initializer: variable
```C
   int a = 1;
   int b = a;   // INVALID
   
   int main() {
       return b;
   }
```

Duplicate object
```C
    int x;
    int x;     // INVALID
    
    int main() {
        return 0;
    }
```

Object/function object
```C
    int f;
    
    int f() {
        return 1;
    }
```

function/object conflict
```C
    int f();
    
    int f;    // INVALID
    
    int f() {
       return 1;
    }
```

pointer initialized to "NULL/0"
```C
    int *p = 0;
    
    int main() {
        int x = 87;
        p = &x;
        return *p;    // expect 87
    }
```

## Out-of-scope for file scope
```C
    int arr[3] = {1,2,3};        // initializer lists
    char s[] = "hello";          // string based array initialization
    int *p = &x;                 // adress constant initializer
    extern int x;                // storage class
    static int x;                // internal linkage
    const int x = 3;             //qualifiers
```