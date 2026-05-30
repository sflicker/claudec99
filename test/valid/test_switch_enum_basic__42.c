typedef enum {
    AST_TRANSLATION_UNIT,
    AST_FUNCTION_DECL
} ASTNodeType;

int main() {
    ASTNodeType nodeType = AST_TRANSLATION_UNIT;

    switch (nodeType) {
        case AST_TRANSLATION_UNIT:
            return 42;
        case AST_FUNCTION_DECL:
            return 1;
        default:
            return 2;
    }
}
