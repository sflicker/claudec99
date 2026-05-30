typedef enum {
    AST_TRANSLATION_UNIT,
    AST_FUNCTION_DEF,
    AST_RETURN_STMT
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
} ASTNode;

int classify(ASTNode *node) {
    switch (node->type) {
        case AST_TRANSLATION_UNIT:
            return 10;
        case AST_FUNCTION_DEF:
            return 20;
        case AST_RETURN_STMT:
            return 12;
        default:
            return 0;
    }
}

int main() {
    ASTNode node;
    node.type = AST_RETURN_STMT;
    return classify(&node);
}
