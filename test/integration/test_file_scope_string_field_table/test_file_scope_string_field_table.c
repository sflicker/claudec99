int puts(char *s);

struct TokenMapEntry {
    char *name;
    int kind;
};

struct TokenMapEntry keywords[] = {
    {"if",   1},
    {"else", 2}
};

int main() {
    puts(keywords[0].name);
    return keywords[1].kind;
}
