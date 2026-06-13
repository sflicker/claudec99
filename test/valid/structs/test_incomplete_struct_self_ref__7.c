typedef struct ASTNode ASTNode;

struct ASTNode {
    int kind;
    ASTNode *left;
    ASTNode *right;
};

int main() {
    struct ASTNode node;
    node.kind = 7;
    node.left = 0;
    node.right = 0;
    return node.kind;
}
