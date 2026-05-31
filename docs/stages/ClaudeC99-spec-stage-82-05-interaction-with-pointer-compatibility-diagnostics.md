# ClaudeC99 stage 82-05 interaction with pointer compatibility diagnostics

## Task
Make sure existing const pointer warning/error behavior still works when the const comes from a member type

example
```C
struct S {
    const char *s;
};

int main() {
    struct S s;
    char *q;

    s.p = "hello";
    q = s.p;     // potential warning or error depending on settings and implementation
    return q[0] == 'h';
}
```

Implementation notes
Struct member type storage
  - when adding a member to a struct/union field table, do not string qualifiers
  for the above example
  BAD: member type parsed as const char *; stored as char *
  GOOD: member type parser as pointer to const char; stored exactly as pointer to const char

Member access result type
For:
```C
s.member
p->member
```
the result expression type should be the stored member type, including qualifiers

So if:
```C
struct S {
    const int x;
};
```
then:
```C
s.x;
```

has type: `const int lvalue`
and assignment should be rejected.

**Const object plus member access**
if the base object itself is const:
```C
const struct S s;
s.x = 1;
```
then all member access through s should be non-modifiable even if the member type itself is not const.

## Tests
Const char pointer member
```C
struct S {
    const char *name;
};

int main(void) {
    struct S s;
    s.name = "ClaudeC99"'
    return s.name[0] == 'C';       //expect 1
}
```

Const scalar member initialized by aggregate initializer
```C
struct S {
    const int code;
};

int main(void) {
    struct S s = {42};

    return s.code;    // expect 42
}
```

Volatile member Accepted
```C
struct S {
    volatile int flag;
};

int main(void) {
    struct S s;
    s.flag = 42;
    return s.flag;    // expect 42
}
```

size of const type-name
```C
int main(void) {
    return sizeof(const char *) == 8;   //expect 1
}
```

## Invalid tests

Assignment to const member
```C
struct S {
    const int code;
};

int main(void) {
    struct S s = {1};

    s.code = 2;      // invalid
    return s.code;
}
```

Assignment through pointer-to-const member
```C
struct S {
    const char name;
};

int main(void) {
    struct S s;
    s.name = "abc";
    s.name[1] = 'B';    // INVALID
    return 0;
}
```

Const-discard from member
```C
struct S {
    const char *name;
};

int main(void) {
    struct S s;
    char *p;

    s.name = "abc";
    p = s.name;     // INVALID or Warning
    return p[0];
}