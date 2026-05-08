# ClaudeC99 stage 25-03 Indirect function calls

## goal allow calling through fucntion pointers
```C
   int inc(int x) {
       return x + 1;
   }
   
   int main() {
       int (*fp)(int) = inc;
       return fp(4);     // expect 5
   }
```
Also support explicit dereference spelling
```C
    return (*fp)(4);
```

## Requirements
  - Support indirect calls through:
  ```C
      fp(3);
      (*fp)(3); 
  ```
  - Validate
    - Callee expression has function-pointer type
    - argument count matches
    - argument types are compatible
    - return type is used as the call expression type

## Valid Tests
Basic indirect call
```C
    int inc(int x) {
        return x + 1;
    }
    
    int main() {
       int (*fp)(int) = inc;
       return fp(4);    // expect 5
    }
```

Explicit dereference call
```C
   int inc(int x) {
      return x + 1;
   }
   
   int main() {
       int (*fp)(int) = inc;
       return (*fp)(4);       // expect 5
   }
```

Two argument function pointer
```C
    int add(int x, int y) {
        return x + y;
    }
    
    int main() {
        int (*op)(int, int) = add;
        return op(2,3);    // expect 5
    }    
```

Function pointer parameter
```C
    int inc(int x) {
        return x + 1;
    }
    
    int apply(int (*fp)(int), int x) {
        return fp(x);
    }
    
    int main() {
       return apply(int, 6);   // expect 7 
    }
```

Reassignment
```C
    int inc(int x) {
        return x+1;
    }
    
    int dec(int x) {
        return x-1;
    }
    
    int main() {
        int (*fp)(int) = inc;
        int a = fp(10);
        
        fp = dec;
        int b = dec(10);
        
        return a + b;        // expect 20;
    }
```

## Invalid Tests
Non function pointer call
```C
    int main() {
        int x = 3;
        return x(1);    // INVALID
    }
```
Pointer-to-int is not callable
```C
    int main() {
        int x = 3;
        int *p = &x
        return p(1);   // INVALID
    }
```

Wrong argument count
```C
    int add(int a, int b) {
        return a + b;
    }
    
    int main() {
        int (*fp)(int, int) = add;
        add(1);   // INVALID
    }
```

Wrong argument type
```C
    int read(int *p) {
        return *p;
    }
    
    int main() {
        int (*fp)(int*) = read;
        return read(3);    // INVALID
    }
```
