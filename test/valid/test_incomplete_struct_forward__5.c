struct Lexer;

struct parser {
    struct Lexer *lexer;
    int current;
};

int main() {
    struct parser p;
    p.current = 5;
    return p.current;
}
