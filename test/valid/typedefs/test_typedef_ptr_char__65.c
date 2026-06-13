typedef char * String;

int main() {
    char c = 65;
    String p = &c;
    return *p;
}
