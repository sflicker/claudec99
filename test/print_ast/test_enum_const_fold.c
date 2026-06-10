enum Perm {
    PERM_NONE  = 0,
    PERM_READ  = 1 << 0,
    PERM_EXEC  = 1 << 2
};
int main() {
    int p = PERM_EXEC;
    return p - 4;
}
