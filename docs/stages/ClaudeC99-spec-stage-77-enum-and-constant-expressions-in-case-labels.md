# ClaudeC99 stage 77 enum and constant expressions (including simple integer constants expressions) in case labels

## Goal
Allow:
```C
   switch (node->type) {
    case AST_TRANSLATION_UNIT:
        printf("TranslationUnit:\n");
        break;
     ...
    }
```
where AST_TRANSLATION_UNIT is an enum constant

**Semantic Rule**
A case label must be known at compile time.
so for an enum like
typedef enum {
    AST_TRANSLATION_UNIT,
    ...
} AST_NODE_TYPE

AST_TRANSLATION_UNIT can be resolved at compile time to be -> 0

**Reject non-constant value**
```C
int x = 1;

switch(n) {
    case x:     /* invalid */
    ...
```

## case evaluation
cases should be evaluated using a simplified evaluator at compile
time to determine if they resolve to a constant.

Cases that should be supported 
```C
case AST_TRANSLATION_UNIT:
case AST_TRANSLATION_UNIT + 10:
case TOKEN_BASE -1:
case -TOKEN_BASE:
case 'A':
case 42:
```

case evaluator should only use
```text
integer literal
character literal
enum constant
unary + -
binary + -
```
to determine if the case resolves to a constant.

## Out of scope
case evaluations including
  - variables
  - read only variable defined with `const`
  - parentheses
  - multiplication
  - division
  - shifts
  - sizeof
  - casts
  - conditionals
  - complex conversion rules
  - any other not mentioned above in the allowed list

## Tests
Switch case with Enumeration
```C
typedef enum {
    AST_TRANSLATION_UNIT,
    AST_FUNCTION_DECL
} ASTNodeType;

int main() {
    ASTNodeType nodeType = AST_TRANSLATION_UNIT;
    
    switch (nodeType) {
        case AST_TRANSLATION_UNIT:
            return 42;           // expect 42
        case AST_FUNCTION_DECL:
            return 1;
       default:
            return 2;
    }
}
```

Switch case with `+` on Literals
```C
#define BASE 0

int main() {
    int x = 2;
    
    switch(x) {
        case BASE:
            return 0;
        case BASE + 1:
            return 1;
        case BASE + 2:
            return 42;      // expect 42
        default:
            return 3;
    }
}
```

switch case with unary `-`
```C
int main() {
    int x = -3;
    
    switch(x) {
       case 0:
          return 0;
       case -3:
          return 42;    // expect 42
       default:
          return 1;
    }
}
```

Enum in a struct
```C
typedef enum {
    AST_TRANSLATION_UNIT,
    AST_FUNCTION_DEF,
    AST_RETURN_STMT
} ASTNodeType;

typedef struct ASTNode {
   ASTNodeType type;
} ASTNode;

int classify(ASTNode * node) {
   switch(node->type) {
       case AST_TRANSLATION_UNIT:
           return 10;
       case AST_FUNCTION_DEF:
           return 20;
       case AST_RETURN_STMT:
           return 12;
       default 0;
   }
}

int main() {
    ASTNode node;
    node.type = AST_RETURN_STMT;
    return classify(&node);      // expect 12
}
```

Explicit enum values
```C
enum TokenKind {
    TOKEN_EOF = 0,
    TOKEN_INT = 10,
    TOKEN_NAME = 42
};

int main(void) {
    int kind;
    kind = TOKEN_NAME;
    
    switch (kind) {
        case TOKEN_EOF:
            return 1;
        case TOKEN_INT:
            return 2;
        case TOKEN_NAME:
            return 42;     // expect 42
    }
    return 0;   
}
```

Character literal
```C
int main(void) {
    char c;
    c = 'A';
    
    switch(c) {
        case 'A':
            return 42;   // expect 42
        default:
            return 0;
    }
}
    
```

## Invalid Tests
Non constants in switch
```C
int main() {
   int x = 10;
   int y = 20;
   int a = 10;
   
   switch (a) {
       case x:              // INVALID
          return 1;
   }
   return 0;
}
```