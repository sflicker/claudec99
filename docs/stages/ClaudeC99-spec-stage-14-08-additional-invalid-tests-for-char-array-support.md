# ClaudeC99 Stage 14-08

## Task
  - Add invalid tests and diagnostic cleanup for string literal and char array initialization support

## Requirements
  - Add invalid tests for malformed string literals
  - Add invalid tests for incomplete string literal assignments
  - Add invalid tests for invalid char array initialization from string literals
  - Ensure diagnostics are clear and stable enough for test expectations.
  - Ensure existing valid tests continue to pass.

## Invalid cases

### Reject malformed string literals

```C
    int main() {
        char *s = "unterminated;         
    }
    
    int main() {
        char *s = "Hello
    world";
        return 0;
    }
```

### Reject unsupported escape sequences:

```C
    int main() {
        char *s = "\q";
        return 0;
    }
    
    int main() {
        char * = "\x41";
        return 0;
    }
```

### Reject incompatible pointer initialization:

```C
    int main() {
        int *p = "hello";
        return 0;
    }
```

### Reject assigning string literals directly to integer variables:

```C
    int main() {
        int x = "hello";
        return x;
    }
```

### Reject assigning string literals directly to scalar char variables:
```C
    int main() {
        char c = "x";
        return c;
    }
```

### Reject too-small char arrays:
```C
    int main() {
        char s[2] = "hi";
        return 0;
    }
```

### Reject omitted array size without string literal initialization:
```C
    int main() {
        char s[];
        return 0;
    }
```

### Reject omitted array size with non-string initializer
```C
    char s[] = 123;
    return 0;
```

### Reject non-char inferred arrays initialized from string literals
```C
    int main() {
        int s[] = "hi";
        return 0;
    }
```

### Reject non-char explicit arrays initialized from string literals
```C
    int main() {
        int s[3] = "hi";
        return 0;
    }
```

## Out of scope
  - `const`
  - rejecting writes through string literal pointers
  - adjacent string literal concatenation
  - character literals such as `a`
  - hex escapes
  - octal escapes
  - Unicode escapes
  - `printf`
  - variadic functions
