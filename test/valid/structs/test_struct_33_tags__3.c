struct S01 { int v; }; struct S02 { int v; }; struct S03 { int v; };
struct S04 { int v; }; struct S05 { int v; }; struct S06 { int v; };
struct S07 { int v; }; struct S08 { int v; }; struct S09 { int v; };
struct S10 { int v; }; struct S11 { int v; }; struct S12 { int v; };
struct S13 { int v; }; struct S14 { int v; }; struct S15 { int v; };
struct S16 { int v; }; struct S17 { int v; }; struct S18 { int v; };
struct S19 { int v; }; struct S20 { int v; }; struct S21 { int v; };
struct S22 { int v; }; struct S23 { int v; }; struct S24 { int v; };
struct S25 { int v; }; struct S26 { int v; }; struct S27 { int v; };
struct S28 { int v; }; struct S29 { int v; }; struct S30 { int v; };
struct S31 { int v; }; struct S32 { int v; }; struct S33 { int v; };

int main() {
    struct S01 a;
    struct S33 b;
    a.v = 1;
    b.v = 2;
    return a.v + b.v;
}
