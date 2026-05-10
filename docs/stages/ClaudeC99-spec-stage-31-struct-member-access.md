# ClaudeC99 stage 31 - struct member access

## Tasks
  - add support for struct member access using both `.` and `->`
  - add field access as both rvalue and lvalue

## Tokens
Add These if not already
`.`
`->`

## Grammar update
```ebnf
<postfix_expression> ::= <primary_expression> 
                    { "[" <expression> "]"                     
                      | "++" 
                      | "--"
                      | "(" [ <argument_expression_list> ] ")"     
                      | "." <identifier>
                      | "->" <identifier> }
```
## Tests
**Core Tests**
```C
    struct Point {
        int x;
        int y;
    };
    
    int main() {
        struct Point p;
        p.x = 3;
        p.y = 4;
        return p.x + p.y;   // expect 7
    }
```

```C
    struct Point {
        int x;
        int y;
    };
    
    int main() {
        struct Point p;
        struct Point *q = &p;
        
        q->x = 10;
        q->y = 20;
        
        return q->x + q->y;  // expect 30
    }
```

**Padding Alignment Test**
```C
    struct Mixed {
        char c;
        int i;
    };
     
    int main() {
        struct Mixed s;
        s.c = 5;
        s.i = 10;
        return s.c + s.i; // expect 15
    } 
```

### Invalid Tests
```C
    main() {
        int x;
        return x.y;    // INVALID
    }
```

```C
    struct Point { 
        int x;
    }
    
    int main() {
        struct Point p;
        return p.y;      // INVALID
    }
```

```C
    struct Point {
        int x;
    };
    
    int main() {
        struct Point p;
        return p->x;     // INVALID
    }
```

### Out of Scope
  - struct assignment
  - struct parameters
  - struct return values
  - struct initializers
  - nested struct/union edge cases
