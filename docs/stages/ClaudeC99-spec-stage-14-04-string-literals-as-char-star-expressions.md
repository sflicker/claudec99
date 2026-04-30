# ClaudeC99 Stage 14-04

## Task
  - Allow string literals to be used as expressions that can initialize and assign to `char *` variables

## Requirements
  - Treat string literal expressions as array objects of type `char[N]`, where `N` is literal byte length + 1.
  - Allow string literals to decay to `char *` in ordinary expression contexts
  - Allow initialization of `char *` variables from string literals
  - Allow assignment of string literals to `char *` variables
  - Ensure indexing through the resulting pointer works
  - Existing tests must continue to pass

## Tests to add
```C
    int main() {
       char *s = "hi";
       return s[0];   // expect 104
    }
```

```C
    int main() {
        char *s = "hi";
        return s[1];    // expect 105
    }
```

```C
    int main() {
        char *s;
        s = "hi";
        return s[1];   // expect 105
    }
```

```C
    int main() {
        char *s = "";
        return s[0];  // expect 0
    }       
```

```C
    int main() {
        char * s = "abc";
        return s[2];    // expect 99
    }
```

```C
    int main() {
        char *s = "abc";
        return *(s + 1);
    }
```

Invalid Tests:
```C
    int main() {
        int *p = "hello";
        return 0;
```

### Out of scope
  - Escape characters
  - Char array initialization from string literals
  - `char s[] = "hi";`
  - Adjacent string literal concatenation
  - `const`
  - Rejecting writes through string literal pointers
  - Libc calls such as `puts("hi")`
