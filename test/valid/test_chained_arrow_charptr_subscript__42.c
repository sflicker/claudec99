struct Entry {
    char *name;
};

int main(void) {
    struct Entry e;
    struct Entry *p;

    p = &e;
    p->name = "ABC";

    /* 'A' + 'B' + 'C' = 65 + 66 + 67 = 198; 198 - 156 = 42 */
    return p->name[0] + p->name[1] + p->name[2] - 156;
}
