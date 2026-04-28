# ClaudeC99 Stage 13.4

## Task
Add pointer arithmetic for pointer + integer, integer to pointer and pointer - integer

## Requirements
  - support pointer arithmetic in additive expressions.
  - scale integer offsets by the pointed to type size.
  - preserve existing integer arithmetic behavior.

## Type Rules
  - `T * + integer -> T *`
  - `integer + T * -> T *`
  - `T * - integer -> T *`
  - `pointer + pointer` is invalid
  - `integer - pointer` is invalid

### Codegen Rules
For: `p + n`
  - Generate address equivalent to:
    `p + n * sizeof(*p)`

For: `p - n`
  - Generate `p - n * sizeof(*p)`

### Tests
```C
    int main() {
        int a[3];
        int *p = a;
        *(p + 1) = 42;
        return a[1];  // expect 42
    }
```

```C
    int main() {
        long a[2];
        long *p = a;
        *(p + 1) = 42;
        return a[1];  // expect 42
    }
```

```C
    int main() {
        char a[2];
        char *p = a;
        *(1 + p) = 9;
        return a[1];  // expect 9
    }
```

```C
    int main() {
        long a[3];
        long *p = &a[2];
        *(p - 1) = 17;
        return a[1];  // expect 17
    }
```

Invalid Tests:
```C
   int main() {
       int a[2];
       int *p = a;
       int *q = a;
       return p + q;  //ERROR
```

```C
   int main() {
      int a[2];
      int *p = a;
      return 1 - p;  // ERROR
```

### Out of scope
  - pointer - pointer
  - pointer comparisons
  - pointer arithmetic on `void *`
  - bounds checking