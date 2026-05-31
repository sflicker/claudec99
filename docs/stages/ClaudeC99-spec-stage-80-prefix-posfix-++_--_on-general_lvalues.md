# ClaudeC99 Stage 80 - Prefix/Postfix ++/-- on general lvalues

## Goal - Expand ++ / -- so they work on any modifiable lvalue, not just bare identifiers

Currently Accepted
```C
i++
--i
```

Currently rejected
```C
parent->child_count++;
p->len++;
g->data[g->len++] = c;
--x->n;
arr[i]++;
```

The two real self-compile blockers are classic C idioms:
```C
parent->children[parent->child_count++] = child;
g->data[g->len++] = c;
```

Required semantic rule
The operand of prefix/postfix ++ or -- must be a modifiable lvalue or arithmetic or pointer type.

Allow:
```C
i++;
arr[i]++;
p->len++;
tokens[i].kind++;
(*p)++;
--x->n;
```

Reject:
```C
42++;
(a + b)++;
const int x = 1; x++;
```
also reject non-modifiable things like array/functions if those appear as operands.


for postfix ++ / -- should attach to whatever postfix expression has already been built.

For prefix do not require the operand to be an identifier. Require it to be a valid
lvalue after parsing/type checking.

## Tests

Example based on ast.c pattern
```C
struct Parent {
    int children[4];
    int child_count;
};

int main(void) {
    struct Parent p;
    struct Parent *parent;
    
    parent = &p;
    parent->child_count = 0;
    
    parent->children[parent->child_count++] = 42;
    
    return parent->children[0] + parent->child_count;  // expect 43
}
```

Example based on preprocessor.c pattern
```C
struct Grow {
    char data[0];
    int len;
};

int main(void) {
    struct Grow g;
    
    g.len = 0;
    g.data[g.len++] = 'A';
    g.data[g.len++] = 'B';
    
    // 'A' + 'B' = 131
    // 131 - 89 = 42
    return g.data[0] + g.data[1] - 89;   // expect 42
}
```

Member postfix increment
```C
struct Buffer {
    int cap;
};

int main(void) {
    struct Buffer b;
    struct Buffer *p;
    
    p = &b;
    p->cap = 41;
    p->cat++;
    
    return p->cap;    // expect 42
```

prefix decrement on arrow member
```C
struct Counter {
    int n;
};

int main(void) {
    struct Counter c;
    struct Counter *p;
    
    p = &c;
    p->n = 43;
    
    return --p->n;     // expect 42
}
```

array element increment
```C
int main(void) {
    int xs[2];
    
    xs[1] = 41;
    xs[1]++;
    
    return xs[1];    //expect 42
}
```

Postfix result is old value
```C
int main(void) {
    int x;
    int y;
    
    x = 41;
    y = x++;
    
    // x = 42, y = 41
    return x + y - 40;    // expect 43
}
```

prefix result is new value
```C
int main(void) {
    int x;
    int y;
    
    x = 41;
    y = ++x;
    
    return y;    // expect 42
}
```

## Invalid Tests
```C
int main(void) {
    return 42++;    //INVALID
}
```

```C
int main(void) {
    int a;
    int b;

    a = 1;
    b = 2;

    (a + b)++;  // INVALID

    return 0;
}
```

```C
int main(void) {
    const x = 1;
    x++;     // INVALID
    return x;
}
```

## Completion Criteria
  - prefix and postfix ++/-- accept general modifiable lvalues
  - identifier cases still work
  - member lvalue work; `p->len++`, `--p->len`
  - subscript lvalues work; `arr[i]++`
  - nested cases work: `g->data[g->len++] = c`
  - postfix returns old value
  - prefix returns new value
  - pointer ++/-- continues to scale properly
  - invalid non-lvalues are rejected cleanly
  - full test suite passes

