# ClaudeC99 stage 82-02 const-qualified member lvalue rules

## Feature
Member access should preserve const-qualification

If a member is declared
```C
struct S {
    const int x;
};
```
Then:
```C
s.x = 2;
```
should be rejected

Similarly:
```C
struct S {
    const char *p;
};
means s.p itself is assignable if the struct object is mutable:
```C
s.p = "hello";   //OK
```
But writing through s.p should be rejected:
```C
*s.p = 'H';    // INvalid: pointed-to char is const 
```

## Test
Pointer-to-const member assignment is OK:
```C
struct S {
    const char *p;
};

int main() {
    struct S s;
    s.p = "hello";
    return s.p[1] == 'e';    // expect 1
}
```

## Invalid Test
Writing through pointer-to-const member is invalid:
```C
struct S {
    const char *p;
};

int main(void) {
    struct S s;
    s.p = "hello";
    s.p[0] = 'H';       // INVALID
    return 0;
}
```