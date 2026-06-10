# ClaudeC99 Stage-22-01 File scope uninitialized object declarations

## goals
  - add support for file scope object declarations
  - this stage will only handle uninitialized objects

Support declarations like these
```C
    int g;
    char c;
    short s;
    long l;
    int * p;
    int arr[10];
    
    int main() {
        g = 42;
        return g;
    }
```


## Requirements
  - parse file-scope object declarations
  - add global variables to a global symbol table
  - emit storage for globals
  - allow functions to read and write globals
  - preserve local-variable shadowing

## Codegen
For NASM, uninitialized globals can go in .bss and begin with zero value.
```asm
  g: resd 1       ; int g
  c: resb 1       ; char c
  s: resw 1       ; short s
  l: resq 1       ; long l
  p: resq 1       ; int * p
  arr; resd 10    ; int arr[10]
```

Then access them from code with RIP-relative addressing

```asm
    mov eax, [rel g]
    mov [rel g], eax
```

## Tests
### Valid Tests

Basic test
```C
    int x;
    
    int main() {
        x = 7;
        return 7;     //expect 7
    }
```

Shadowing Test
```C
    int x;
    
    int main() {
        int x = 3;
        return x;     // expect 3
    }
```

Globals persist across function calls
```C
    int counter;
    
    int inc() {
        counter = counter + 1;
        return counter;
    }
    
    int main() {
        inc();
        inc();
        return counter;    //expect 2
    }
```

add additional tests to cover other types, invalid cases, AST, and asm tests