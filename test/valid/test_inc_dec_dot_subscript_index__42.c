/* Stage 80: postfix ++ inside a subscript index applied to a dot-access
 * array member (preprocessor.c growth idiom).
 * 'A' + 'B' = 131; 131 - 89 = 42. */
struct Grow {
    char data[8];
    int len;
};

int main(void) {
    struct Grow g;

    g.len = 0;
    g.data[g.len++] = 'A';
    g.data[g.len++] = 'B';

    return g.data[0] + g.data[1] - 89;
}
