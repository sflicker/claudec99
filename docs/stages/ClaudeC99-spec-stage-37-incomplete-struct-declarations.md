# ClaudeC99 stage 37

## task
  support incomplete struct forms
  
## Examples

```C
    typedef struct ASTNode ASTNode;
    
    struct ASTNode {
        int kind;
        ASTNode *left;
        ASTNode *right;
    };
```

```C
    struct Lexer;
    struct parser {
        struct Lexer *Lexer;
        int current;
    };
```

Make appropriate changes to enable the above examples to work (may need to example fragments to include a main)