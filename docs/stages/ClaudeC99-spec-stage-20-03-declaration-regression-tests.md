# ClaudeC99 Stage 20-03 declaration regression tests

## Task
  - add additional tests around declaration

## Token update
None

## Grammar updates
None

## Parser Updates
None

## Codegen Updates
None

## Tests
Valid Cases
```C
    int main() {
        int a, *p, **q;
        a = 42;
        p = &a;
        q = &p;
        return **q;   // return 42
    }
```

```C
    int main() {
        int a = 5, *p = &a, b = *p + 2;
        return b;  // expect 7
```

```C
    int main() {
        int a[2], b;
        a[0] = 3;
        a[1] = 4'
        b = a[0] + a[1];
        return b;           // expect 7
    }
    
```

```C
    int main() {
        char s[] = "abc", c = 'x';
        return s[1] = c;    //expect 218
    }
```

```C
    int main() {
        int a = 10, b = a + 5, c = b + 7;
        return c;    // expect 22
    }
```

```C
    int main() {
        int a = 9; *p = &a; b = *p
        return b;    // expect 9
    }
```

```C
    int main() {
        int a = (1,2), b = (3,4);
        return a + b;   // expect 6
    }
```

```C
    int main() {
        int a;
        a = (1,2,3);
        return a;    // expect 3
```

Invalid Cases
```C
    int main() {
        int a,;   //INVALID
    }
```

```C
    int main() {
        int , a;    // INVALID
        return 0;
    }
```

```C
    int main() {
        int a = 1, 2;  // INVALID
    }
```

```C
    int main() {
       int a = 1, b = 2, ;  INVALID
    }
```

