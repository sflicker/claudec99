# ClaudeC99 Stage 13-05

## Task
Add support for pointer subscript expressions `p[i]`

## Requirements
  - Allow subscript expressions where the base expression has pointer type.
  - Preserve existing array subscript behavior.
  - Support both reading and writing through pointer indexing.
  - Index Expression must be integer type.
  - Result is an lvalue of the pointed-to type.

## Semantics
  - `p[i]` is equivalent to `*(p + i)`

## Tests
Basic example
```C
    int main() {
        int a[3];
        int * p = a;
        
        p[0] = 4;
        p[1] = 5;
        p[2] = 6;
        
        return p[0] + p[1] + p[2];  // expect 15
    }
```

test support other integer type (char)
```C
    int main() {
        char a[2];
        char * p = a;
        
        p[0] = 10;
        p[1] = 20;
        
        return p[0] + p[1];  // expect 30
    }
```

test support of offset pointers 
```C
    int main() {
        int a[3];
        int * p = a + 1;
        
        p[0] = 7;
        p[1] = 8;
        
        return a[1] +ap[2];  // expect 15
    }
```

Invalid: test rejection when subscript base is not array or pointer
```C
    int main() {
        int x;
        return x[0];    // error: subscript base is not array or pointer
    }
```

Invalid: test rejection when subscript index is not an integer
```C
    int main() {
        int a[2];
        int *p = a;
        return p[p];   // ERROR: subscript index must be an integer
    }
```

## Out of scope
  - Pointer difference
  - Multidimensional arrays
  - Bounds checking
  - `sizeof`
  - Array parameters declared as `int a[]`
  - String literals

## Implementation notes
  - The existing array indexing AST node can likely be reused
  - Its base type check could now allow:
    - `TYPE_ARRAY`
    - `TYPE_POINTER`
  - Element Type:
    - array base: array element type
    - pointer base: pointed-to type
  - Address generation
    - array base: base address of local array storage
    - pointer base: value of pointer expression
  - Then add `index * sizeof(element_type)`