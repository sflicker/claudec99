struct Token {
    int kind;
    int value;
};

int main(void) {
    struct Token tokens[2];

    tokens[0].kind = 10;
    tokens[1].kind = 32;

    return tokens[0].kind + tokens[1].kind;
}
