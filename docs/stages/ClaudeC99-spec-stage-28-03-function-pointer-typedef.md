# ClaudeC99 Stage 28-03 Function Pointer typedef

## Task
  - Extend typedef to support function pointers

## Tests

Core Test
```C
    typedef int (*BinaryFn)(int, int);
    
    int add(int a, int b) {
        return a + b;
    }
    
    int main() {
        BinaryFn f = add;
        return f(2,3);     // expect 5;
    }
```

assignment after declaration
```C
    typedef int (*BinaryFn)(int, int);
    
    int add(int a, int b) {
        return a + b;
    }
    
    int main() {
        BinaryFn f;
        f = add;
        return (*f)(2,3);     // expect 5;
    }
```

multi-declarator
```C
    typedef int (*Fn)(int);
    
    int inc(int x) {
        return x+1;
    }
    
    int main() {
        Fn a, b;
        a = inc;
        b = a;
        return b(6);     // expect 7
    }
```