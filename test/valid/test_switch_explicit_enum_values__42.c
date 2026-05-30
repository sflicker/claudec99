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
            return 42;
    }
    return 0;
}
