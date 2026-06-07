# ClaudeC99 stage 95-09 remove statis char array ast/parsing

## Task
Change ast.h so 

```C
typedef struct ASTNode {
    ASTNodeType type;
    char value[MAX_NAME_LEN];
    ...
} ASTNode;
```

to 
```C
typedef struct ASTNode {
    ASTNodeType type;
    const char * value;
    ...
} ASTNode;
```

instances where token->value is memcpy to astnode->value should be replaced with assignments like
so 

memcpy(astnode->value, token->value, token->value_len)
astnode->value[token->value_len] = '\0';

should be replaced with

astnode->value = token->value;

And copies of one node->value to another should be updated as well.

A check should be made that all astnode->value bytes are originating from the lexer (or another astnode-value that orginates from the lexer).

## Tests
tests can be added to ensure adequate test coverage

After the stage is finished the fixed-capacity-inventory.md document should be updated.


