typedef enum {
    PERM_NONE  = 0,
    PERM_READ  = 1 << 0,
    PERM_WRITE = 1 << 1,
    PERM_EXEC  = 1 << 2
} Perm;
int main() {
    Perm p = PERM_READ | PERM_WRITE;
    return (int)p - 3;
}
