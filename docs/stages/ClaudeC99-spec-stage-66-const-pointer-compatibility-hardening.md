# ClaudeC99 stage 66 - const pointer compatibility hardening

## Tasks
   - Add a warning for code that discards a `const` pointer.
   - Add a flag that turns warnings into errors (like -Werror or possible something else). I'll refer to it here as warning-to-errors 
     since i'm not actually specifying the actual flag name.

Example
```C
int main(void) {
    const char *p;
    char *q;
    q = p;            // this should generate a warning related to discarding a `const` qualifier.

    return 0;
}

```

## Requirements
   - add a flag that turns warnings into errors
   - add a warning if a const is being discarded
   - add an error if a const pointer is being assigned.
   - allow assignments that add const

## Test
Use the above example that discards `const` and create two tests. One without the warnings-to-error flag would verify compiling the
file generates an error. And the second test would have the warnints-to-error flag set and the test should 
verify an error was generated.

Other tests
```C
int main() {
    char *q;
    const char *p;
    p = q;         // this should not generate a warning or error since const is being added.
    return 0;     // expect 0
}
```

```C
int main() {
    const int x = 1;
    const int *p = &x;
    *p = 2;               // THIS should always generate an ERROR. "assignment through pointer to const"
    return 0;
}
```

```C
int main() {
    int x;
    int * const p = &x;
    p = &x;                // this should always generate an ERROR.
    return 0; 
}
```

```C
int main() {
    const int x = 1;
    int *p;
    p = &x;        // this should generate a warning. Or an error if the warnings-to-error flag is set.
    return 0;
}
```