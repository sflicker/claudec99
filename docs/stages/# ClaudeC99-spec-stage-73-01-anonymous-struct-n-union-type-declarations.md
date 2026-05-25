# ClaudeC99 Stage 73-01 anonymous struct/union type declarations

# Task
  - support anonymous aggregate types used directly in declarations
Example
```C
struct {
    int x;
    int y;
} p; 

union {
    int i;
    char c;
} v;
```

Also support typedefs
```C
typedef struct {
    int x;
    int y;
} Point;

typedef union {
    int i;
    char c;
} Value;
```

Grammar Updates
For structs/unions, allow no tag when a body is present.
```ebnf
<struct_specifier> ::= "struct" [ <identifier> ] "{" <struct_declaration_list> "}"
                     | "struct" <identifier>

<union_specifier> ::= "union" [ <identifier> ] "{" <struct_declaration_list> "}"
                    | "union" <identifier>
```
Constraint:
  - for struct; without tag and without body is invalid
  - for union; without tag and without body is invalid

Valid examples:
```C
struct { int x; } p;
union { int i; } u;
```

These are invalid
```C
struct *p;  /* no tag, no body */
union *p;   /* no tag, no body */
```

## Type identify
Each anonymous struct/union definition creates a unique type.
so
```C
    struct { int x; } a;
    struct { int x; } b;
    
    a = b;    /* invalid: different anonymous struct types */
```
even though the layouts match, the are different C types

## anonymous types can be used with typedef
Example
```C
typedef struct {int x;} S;
S a;
S b;
a = b;
```
because both use the same typedef-defined anonymous type.

## Out of scope
  - Anonymous struct/union members

## Tests
```C
int main() {
    struct {
        int x;
        int y;
    } p;
    
    p.x = 10;
    p.y = 32;
    return p.x + p.y;   //expect 42
} 
```

```C
int main() {
    struct {
        int x;
        int y;
    } p, q;
    
    p.x = 10;
    p.y = 32;
    q = p;
    return q.x + q.y;   //expect 42
} 
```

```C
typedef struct {
                  int x;
                  int y;
               } P;
               
int main() {
    P a;
    a.x = 10;
    a.y = 32;
    return a.x + a.y;    //expect 42
}
```

Anonymous union examples
```C
int main() {
    union { int a;
        char b;
    } u;
    u.a = 42;
    return u.a;   // expect 42
```

```C
int main() {
    union { int a;
            char b;
          } u, v;
    u.a = 42;
    v = u;
    return v.a;    // expstec 42
```

```C
typedef union { int a;
                char b;
              } U;
              
int main() {
    U u;
    u.a = 42;
    return u.a;     // expect 42              
```

```C
typedef union { int a;
                char b;
              } U;
              
int main() {
    U u;
    U v;
    u.a = 42;
    v = u;
    return v.a;     // expect 42              
```

## Invalid tests

```C

int main() {
    struct *p;  INVALID:
    return 0;
}
```

```C

int main() {
    union *p;  INVALID:
    return 0;
}
```

```C
int main9() {
    struct { int x; } a;
    struct { int y; } b;
    
    a = b;      // INVALID
    return 0;
```