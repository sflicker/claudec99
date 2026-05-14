/* bench_claudec99.c */

unsigned long mix(unsigned long x) {
    x = x * 1103515245 + 12345;
    x = x ^ (x >> 11);
    x = x + (x << 7);
    x = x ^ (x >> 13);
    return x;
}

unsigned long run_one(unsigned long seed, int rounds) {
    unsigned long x = seed;
    int i = 0;

    while (i < rounds) {
        x = mix(x + (unsigned long)i);

        if ((x & 1) != 0) {
            x = x * 3 + 1;
        } else {
            x = x >> 1;
        }

        i = i + 1;
    }

    return x;
}

int main(int argc, char **argv) {
    unsigned long total = 0;
    unsigned long seed = (unsigned long)argc + 1;
    int i = 0;

    while (i < 8) {
        total = total + run_one(seed + (unsigned long)(i * 97), 2000000);
        i = i + 1;
    }

    return (int)(total & 255);
}

