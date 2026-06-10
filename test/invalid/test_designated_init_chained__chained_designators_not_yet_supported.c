struct Inner { int v; };
struct Outer { struct Inner a; };
int main() { struct Outer o = { .a.v = 1 }; return 0; }
