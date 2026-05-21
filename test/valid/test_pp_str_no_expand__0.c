#define NAME Bob
#define STR(x) #x

int strcmp(const char *, const char *);

int main(void) {
    return strcmp(STR(NAME), "NAME");
}
