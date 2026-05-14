int main() {
    char *s;
    void *items[1];

    s = "abc";
    items[0] = s;

    return items[0][0]; // REJECT: items[0] is void *
}
