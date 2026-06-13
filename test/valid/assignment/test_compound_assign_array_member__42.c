/* Stage 79: general-lvalue compound assignment on an array element's member. */
struct Token {
    int kind;
};

int main(void) {
    struct Token tokens[2];
    int i;

    i = 1;
    tokens[i].kind = 40;
    tokens[i].kind += 2;

    return tokens[1].kind;      /* expect 42 */
}
