/* Stage 80: classic self-compile idiom — postfix ++ inside a subscript
 * index applied to an arrow-access array member.
 * children[0] receives 42, child_count becomes 1; 42 + 1 = 43. */
struct Parent {
    int children[4];
    int child_count;
};

int main(void) {
    struct Parent p;
    struct Parent *parent;

    parent = &p;
    parent->child_count = 0;

    parent->children[parent->child_count++] = 42;

    return parent->children[0] + parent->child_count;
}
