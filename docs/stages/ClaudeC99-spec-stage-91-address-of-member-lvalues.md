# ClaudeC99 Address of member lvalues

## Task
Support address of member lvalues
examples
```C
&s.member
&p->member
&arr[i].member
```

The unary `&` should work on any addressable lvalue, not just identifiers or dereference expressions.

`&expr` should check that `expr` is an lvalue and then emit the address of that expression instead of loading
its value.

## Tests
```C
struct S {
    int x;
};

int main(void) {
    struct S s;
    int *p;

    s.x = 42;
    p = &s.x;

    return *p;   // expect 42
}

arrow should also work
```C
struct S {
    int x;
};

int main() {
    struct S s;
    struct S *sp;

    int *p;

    sp = &s;
    sp->x = 42;
    p = &sp->x;

    return *p;   // expect 42
}
