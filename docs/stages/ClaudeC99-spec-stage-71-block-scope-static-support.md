# ClaudeC99 Stage 71 - block-scope static support

## Task
  - Support block-scope `static` variables
  - Remove the current error message "error: storage class specifier not allowed in block scope" for the `static`
    case. However leave this error message for the other cases, e.g. `extern` at block scope.

A block-scope `static` variable has
```text
block scope
static storage duration
persistent values across function class
internal compiler-generated storage
zero initialization if no initializer is provided
```
Example: the following forms in f() would be persisted between calls.
```C
int f() {
    static int x;
    staiic int x = 3;
    static char c;
    static long long n;
    static int *p;
    static const char *s = "Hello";
    ...
}

int main() {
   f();
   ...
}
```

Working example
```C
int f(void) {
    static int x = 0;
    x = x + 1;
    return x;
}

int main() {
    int i;
    int sum = 0;
    for (i=0;i<5;i++) {
        sum += f();
    }
    return sum;     // expect 15
}
```

Support nested block-scope static as well.
Example:
```C
int f(void) {
    if (1) {
        static int x;
        x = x + 1;
        return x;
    }
    return 0;
}
```

A local static should still obey block scoping and shadowing:
```C
int f(void) {
   static int x;
   {
       static int x;
       x = 10;     /* inner static */
   }
   x = 20;
}
```

## Out of scope
  - static local arrays
  - static local structs
  - static local aggregate initializers
  - static local compound literals
  - thread-local storage
  - dynamic initialization

## Semantics
A block-scope `static` declaration should create an object with a unique internal symbol, for example
```C
int f(void) {
    static int x;
}
```
would emit something of the form
```asm
section .bss       ;; could be in .data if an initializer was present.
.Lstatic_f_x_0: resd 1
```

initialization rules
Uninitialized local static:
```C
int f(void) {
    static int x;
    ...
}
```
Should be stored in the .bss segment and start as zero

Initialized local static:
```C
int f(void) {
    static int x = 3;
    ...
}
```
should be stored in the .data segment

Only constant initializers are allowed for static so something like this should be rejected
```C
   int f(void) {
       int y = 3;
       static int x = y;   /* invalid */
   }
```

## Tests
Basic persistence
```C
int next(void) {
    static int x;
    x = x + 1;
    return x;
}

int main() {
    return next() + next();    // expect 3
}
```

Explicit initializer
```C
int next(void) {
   static int x = 40;
   x = x + 1;
   return x;
}

int main() {
    return next() + next();   // expect 83
}
```

separate functions have separate statics
```C
int a(void) {
    static int x;
    x = x + 1;
    return x;
}

int b(void) {
    static int x;
    x = x + 10;
    return x;
}

int main() {
    return a() + b() + a() + b();  // 1 + 10 + 2 + 20 = 33   EXPECT: 33
} 
```

nested static block
```C
int f(void) {
    if (1) {
        static int x;
        x = x + 1;
        return x;
    }
    return 0;
}

int main() {
    return f() + f();   // expect 3
}
```

** Invalid Test
```C
int f(void) {
    int y = 3;
    static x = y;    // INVALID
    return x;
}
```

## Completion Criteria
Stage 71 is complete when
  - block-scope static variables parse correctly
  - they are emitted to static storage, not the stack
  - uninitialized statics are zero-initialized
  - constant initializers work
  - values persist across calls
  - block/scope visibility/shadowing works
  - full test suite passes
