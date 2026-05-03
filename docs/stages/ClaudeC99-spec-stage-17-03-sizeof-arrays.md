# ClaudeC99 Stage-17-03 sizeof arrays

## Task
  - add support for sizeof declared arrays
Examples
```C
    int A[10];
    sizeof(A);    expect 10*sizeof(4) = 40;   
```

## Requirements
  - The sizeof of an array is the number of elements x the sizeof(element_type).
  - sizeof should be evaluated at compiled time and a constant long value.
  - Do not call the normal expression type path in a way that decays arrays to pointers before sizeof sees the array type.

## Codegen
  - sizeof(A) should generate a constant value.
  - do not generate code to evaluate the array expression at runtime.  
```asm
    
    ; sizeof(A)  where A is declared as int A[10];
    mov rax, 40
```

### out of scope
```C
     sizeof(int[10]);       // this form is not handled in this stage. 
     variable length arrays
     struct
     union
     enum
     unsized
     double
     float
     size_t
```

## Tests
```C
    int main() {
        int A[10];
        return sizeof(A);   // expect 40
    }
    
    int main() {
        char C[10];
        return sizeof(C);   // expect 10
    }
    
    int main() {
        short S[10];
        return sizeof(S);   // expect 20
    }
    
    int main() {
        long L[10];
        return sizeof(L);   // expect 80
    }
    
    int main() {
        int *p[10];    // array of pointers to int
        return sizeof(p);   // expect 80
    }
```

## Invalid tests
Array type name form is out of scope for this stage
```C
    int main() {
        return sizeof(int[10]);       // INVALID for this stage.
    }
```