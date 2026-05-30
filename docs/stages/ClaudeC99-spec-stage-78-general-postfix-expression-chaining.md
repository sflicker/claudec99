# ClaudeC99 stage 78 General Postfix Expression Chaining

## Task
  - Allow postfix operators to chain naturally, matching C's postfix-expression model.

## In-Scope
Support these forms
```C
     node->children[i]
     parser->tokens[parser->pos].kind
     table[i]->name[0]
     p->items[i].type
     p->a[i]->x
     arr[i].field
     arr[i].field[j]     
  ```
More generally, support repeated postfix operations
```C
expr[index]
expr.member
expr->member
expr(args)
expr++
expr--
```
Where `expr` can itself already be a postfix expression

given the current grammar for <postfix_expression>

```ebnf
<postfix_expression> ::= <primary_expression> 
                    { "[" <expression> "]"                     
                      | "++" 
                      | "--"
                      | "(" [ <argument_expression_list> ] ")"
                      | "." <identifier>
                      | "->" <identifier> }         
```
The parser starts with a primary expression, then loops while the next token is a postfix operator
Example Parser
```C
parser->tokens[parser->pos].kind
```


## Out of scope
  - compound assignment to member/subscript lvalues
  - postfix ++/-- on member/subscript lvalues
  - full generalized assignable lvalue cleanup
  - anonymous member promotion
  - function-return-pointer-to-array patterns

## Tests
Subscript of arrow member array
```C
struct node {
    int values[3];
};

int main(void) {
    struct Node n;
    struct Node *p;
    
    p = &n;
    p->values[0] = 10;
    p->values[1] = 20;
    p->values[2] = 12;
    
    return p->values[0] + p->values[1] + p->values[2];   // expect 42
}      
```

Subscript of dot member array
```C
struct Node {
    int values[3];
};

int main(void) {
    struct Node n;
    
    n.values[0] = 40;
    n.values[1] = 2;
    
    return n.values[0] + n.values[1];   // expect 42
```

Member access after subscript
```C
struct Token {
   int kind;
   int value;
};

int main(void) {
    struct Token tokens[2];
    
    tokens[0].kind = 10;
    tokens[1].kind = 32;
    
    return tokens[0].kind + tokens[1].kind;  // expect 42
}
```

Arrow/member/subscript chain
```C
struct Token {
    int kind;
};

struct Parser {
    struct Token tokens[4];
    int pos;
};

int main(void) {
    struct Parser parser;
    struct Parser *p;
    
    p = &parser;
    p->pos = 2;
    p->tokens[2].kind = 42;
    
    return p->tokens[p->pos].kind;    // expect 42
}
```

Pointer member subscript
```C
struct Buffer {
    int *items;
};

int main(void) {
    int values[3];
    struct Buffer b;
    struct Buffer *p;
    
    values[0] = 10;
    values[1] = 20;
    values[2] = 12;
    
    b.items = values;
    p = &b;
    
    return p->items[0] + p->items[1] + p->items[2];   // expect 42
```

string/member/subscript chain
```C
struct Entry {
    char *name;
};

int main(void) {
    struct Entry e;
    struct Entry *p;
    
    p = &e;
    p->name = "ABC";
    
    //  'A' + 'B' + 'C' = 65 + 66 + 67 = 198
    // 198 - 156 = 42
    return p->name[0] + p->name[1] + p->name[2] - 156;  // expect 42
    
```

Nested member subscript expression as index
```C
struct Parser {
    int pos;
    int values[4];
};

int main(void) {
    struct Parser p;
    
    p.pos = 4;
    p.values[3] = 42;
    
    return p.values[p.pos];    // expect 42
}
```

## Invalid Tests
Subscript non-array-pointer member
```C
struct S {
    int x;
};

int main(void) {
    struct S s;
    s.x[0];    // INVALID
}
```

Dot member access after non-struct subscript
```C
    int main() {
    int xs[3];
    return xs[0].kind;    // INVALID
}
```

Arrow member access after non-pointer subscript
```C
struct S [
    int x;
};

int main(void) {
    struct S xs[2];
    return xs[0]->x;     // INVALID
}
```

missing member in chained expression
```C
struct S {
    int values[3];
};

int main(void) {
    struct S s;
    return s.values[0].missing;    // INVALID
}
```

## Pretty Printer Test
update pretty printer so chained postfix expressions display the actual
nested structure clearly.

Include a pretty printer test with a sample program like
```C
struct Token {
    int kind;
};

struct Parser {
    struct Token tokens[4];
    int pos;
};

int main(void) {
    struct Parser parser;
    struct Parser *p;
    
    p = &parser;
    p->pos = 2;
    p->tokens[2].kind = 42;
    
    return p->tokens[p->pos].kind;    // expect 42
}
```