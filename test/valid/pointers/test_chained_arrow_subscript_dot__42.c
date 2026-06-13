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

    return p->tokens[p->pos].kind;
}
