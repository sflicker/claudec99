# ClaudeC99 Stage 28-04 array typedefs

## Task
  - Support typedefs for arrays

## Tests

Basic array typedef
```C
    typedef int A[4];
    
    int main() {
        A a;
        a[0] = 1;
        a[1] = 2;
        a[2] = 3;
        a[3] = 4;
        return a[0] + a[1] + a[2] + a[3]; // expect 10
    }
```

Multi-declarator 
```C
    typedef int A[4];
    
    int main() {
        A a, b;
        
        a[0] = 3;
        b[0] = 5;
        
        return a[0] + b[0];  // expect 8
    }
```

sizeof
```C
    typedef int A[4];
    
    int main() {
        A a;
        return sizeof(a);    // expect 16
    }
```

```C
    typedef char Name[8];
    
    int main() {
        Name n;
        return sizeof(n);   // expect 8
    }
```

Pointer to typedef array
```C
    typedef int A[4];
    
    int main() {
        A a;
        A *p = &a;
        
        (*p)[0] = 7;
        return a[0];     // expect 7
    }
```