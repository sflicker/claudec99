# ClaudeC99 Stage 32 - Aggregate initializer Lists

## Task
  - Update grammar and parser/AST to aggregate initializer lists
  - Support partial initializers using zero fill

## Grammar Update
Update
```ebnf
<init_declarator> ::= <declarator> [ "=" <initializer_expression> ]

<initializer_expression> ::= <assignment_expression>
```

To
```ebnf
<init_declarator> ::= <declarator> [ "=" <initializer> ]

<initializer> ::= <assignment_expression>
                 | "{" <initializer_list> [ "," ] "}"
                 
<initializer_list> ::= <initializer> { "," <initializer> }                 
```

## Tests
Array Initializer
```C
    int main() {
        int a[3] = { 1, 2, 3 };
        return a[0] + a[1] + a[2];   // expect 6
    }
```

Struct Initializer
```C
    struct Point {
        int x;
        int y;
    };
    
    int main() {
        struct Point p = {3, 4};
        return p.x + p.y;   // expect 7
    }
```

Partial Initializer, Zero Fill additional elements
```C
    int main() {
        int a[3] = { 5 };
        return a[0] + a[1] + a[2];    // expect 5
    ]
```