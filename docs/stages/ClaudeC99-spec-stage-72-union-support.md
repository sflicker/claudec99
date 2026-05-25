# ClaudeC99 stage 72 union support

## Task
  - Add named `union` support types with layout, storage, member access, assignment, initialization and `sizeof`

## Definition
   - a `union` in c is a user-defined data type that lets you store multiple data types at the same location.
   - Only one member can have a value at any point in time.
   - Changing the value of one member of a `union` will overwrite the others.
   - The sizeof a union is the size of it's largest member
Example
```C
union value {
    int i;
    char c;
};

int main() {
    union value v;
    v.i = 65;
    v.c = 'c';
    return 1;
}   
```

In-scope
Support named union definitions
```C
union Value {
    int i;
    char c;
    long l;
};
```

Support union variable
```C
union Value v;
```

Support member access
```C
v.i = 42;
return v.i;
```

Support pointer access
```C
union Value *p;
p = &v;
p->i = 42;
```

Support sizeof(union T) and sizeof(v)

Support union fields inside structs
```C
struct Node {
    int kind;
    union Value value;
};
```

Support structs inside unions if complete:
```C
struct Pair {
   int a;
   int b;
};

union U {
    int i;
    struct Pair p;
};
```

But if the struct is incomplete it should be rejected inside a union
```C
struct Pair;

union U {
    int i;
    struct Pair p;    // INVALID: incomplete struct object
};
```

## Out of Scope
  - anonymous unions
  - anonymous structs in side unions
  - bit fields
  - designated initializers
  - union by value parameters/returns
  - strict aliasing concerns

## Token update
Add a keyword token for `union`

## Grammar changes
```ebnf
<type_specifier> ::= "void"
                     | <integer_type_specifier> 
                     | "_Bool"
                     | <typedef_name> 
                     | <enum_specifier> 
                     | <struct_specifier>
                     | <union_specifier>

/* only named union specifiers are supported for this stage */                     
<union_specifier> ::= "union" <identifier> [ "{" <struct_declaration_list> "}" ]
                     
```

## Type system
Add a type kind
```C 
TYPE_UNION
```

## Union Layout
```C
size      = max(size of all members), rounded up to union alignment
alignment = max(alignment of all members)
all members offset = 0 
```
Example:
```C
union U { 
    char c;    /* size 1 align 1 */
    int i;     /* size 4 align 4 */
    long l;    /* size 8 align 8 */
```
Result:
```C
sizeof(union U) = 8
alignof(union U) = 8
c offset = 0
i offset = 0
l offset = 0
```

## Member access
Reuse struct member-access machinery where possible;
```C
u.i
p->i
```
Difference from structs
```text
member offset is always 0
member type is the selected field type
lvalue behavior works like struct fields
```

## Assignment and initialization
Support whole union assignment only between the same union type
```C
union U a;
union U b;
a = b;
```

Support first-member initialization
```C
union U {
    int i;
    char c;
};

union U u = {42};   /* initializes first member, i */
```

## Tests
basic union size
```C
union U {
    char c;
    int i;
    long l;
};

int main() {
    return sizeof(union U);    // expect 8
}
```

sizeof works on union variables as well
```C
union U {
    char c;
    int i;
    long l;
};

int main() {
    union U u;
    return sizeof(u);    // expect 8
}
```

Basic member write/read
```C
union U {
    int i;
    char c;
};

int main() {
    union U u;
    u.i = 42;
    return u.i;     // expect 42
}
```

Pointer member access
```C
union U {
    int i;
    char c;
};

int main() {
    union U u;
    union U *p;
    
    p = &u;
    p->i = 42;
    
    return u.i;        // expect 42
}
```

Union inside struct
```C
union U {
    int i;
    char c;
};

struct Box {
    int kind;
    union Value value;
};

int main() {
    struct Box b;
    b.kind = 1;
    b.value.i = 41;
    return b.kind + b.value.i;     // expect 42
}
```

Struct inside union
```C
struct Pair {
    int a;
    int b;
};

union U {
    int i;
    struct Pair p;
};

int main() {
    union U u;
    u.p.a = 10;
    u.p.b = 32;
    return u.p.a + u.p.b;   // expect 42
}
```

Whole union assignment
```C
union U {
    int i;
    char c;
};

int main(void) {
    union U a;
    union U b;
    
    a.i = 42;
    b = a;
    return b.i;    // expect 42
}
```

Global union variable
```C
union U { 
    int i;
    char c;
};

union U u;

int main(void) {
    u.i = 42;
    return u.i;   // expect 42
}
```

First member initialization
```C
union U {
    int i;
    char c;
};

int main(void) {
    union U u = {42};
    return u.i;    // expect 42
}
```

## Invalid Tests
Invalid incomplete union object
```C
union U;

int main(void) {
    union U u;     // INVALID
    return 0;
}
```

## Completion Criteria
```text
named union definitions parse
union layout uses max member size/alignment
sizeof union works
union variables can be local/global
union first member initialization works
member access via . and -> works
unions can appear as struct fields
structs can appear as union fields
whole union assignment works
invalid incomplete union objects are rejected
full test suite passes
```