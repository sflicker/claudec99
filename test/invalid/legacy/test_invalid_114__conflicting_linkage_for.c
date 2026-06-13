static int x;
extern int x;   // INVALID: conflicting linkage

int main() {
    return 0;
}
