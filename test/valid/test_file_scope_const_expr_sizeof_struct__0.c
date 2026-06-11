struct Pair { int a; int b; };
int SZ = sizeof(struct Pair);
int main() { return SZ - 8; }
