# ClaudeC99 Stage 14-07

## Task
  - Verify that existing function declaration all call support works with libc string functions beginning with `puts`.

## Requirements
  - Allow declaring `puts` as
```C
  int puts(char *s); 
  ```
  - Allow passing string literals to `puts`
  - Reuse existing function call support
  - Reuse existing string literal decay to `char *`
  - Ensure generated assembly links successfully with libc using the existing linker path.

### Example
```C
    int puts(char *s);
    
    int main() {
        puts("hello");    // expect "hello" will be output
        return 0;         // expect 0 exit code
    }
```

### Additional Tests
```C
    int puts(char *s);
    
    int main() {
        puts("A");   // expect A will be output
        puts("B");   // expect B will be output
        return 0;    // expect 0 exit code
    }
```

### Out of scope
  - printf
  - variadic functions
  - format strings
  - standard library headers
  - `const char *`
  - `char **`