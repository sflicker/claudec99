# ClaudeC99 Stage 15-03

## Task
  - Verify and complete semantic handling for character literals
  - Character literals should behave as integer constants with type `int`, matching C semantics

## Requirements
  - Confirm `AST_CHAR_INTERAL` has expression type of `TYPE_INT`
  - Confirm character literals work anywhere integer expression currently work:
    - returns
    - variables initialization
    - assignment
    - arithmetic
    - comparison
    - array indexing
    - pointer/array expression where integer expressions are allowed
  - Confirm assignment to smaller integer types uses existing conversion/truncation behavior
  - Confirm no special `char` expression typing is introduced for character literals

## C Semantics
In C, this:
` 'A' ` has type `int`, not `char`. Therefore char c = 'A';
is valid because the integer constant `65` is assigned into a `char`.

## Tests

### Return char literal directly
```C
    int main() {
        return 'A';  // expect 65
    }
```

### Assign to char
```C
    int main() {
        char c = 'A';
        return c;    // expect 65
    }
```

### Assign to int
```C
    int main() {
        int x = 'A';
        return x;    //expect 65
    }
```

### Assign to long
```C
    int main() {
        long x = 'A';
        return x;    expect 65
    }
```

### Arithmetic
```C
    int main() {
        return 'A' + 1;   //expect 66
    }
```

### Comparison
```C
    int main() {
        return 'A' == 65;    // expect 1
    }
```

### Character literal in if condition
```C
    // expect 7
    int main() {
        if ('A') {
            return 7;
        }
        return 3;
    }
```

### NUL character in condition
```C
    // expect 3
    int main() {
        int ('\0') {
            return 7;
        }
        return 3;
    }
```

### String array mutation
```C
   int main() {
       char s[] = "hi";
       s[0] = 'H';
       return s[0];   // expect 72
   }
```