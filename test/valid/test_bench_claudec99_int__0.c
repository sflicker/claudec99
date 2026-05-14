/* bench_claudec99_int.c */

int mix(int x) {
    x = (x * 1103 + 12345) & 32767;
    x = x ^ (x >> 5);
    x = (x + (x << 3)) & 32767;
    x = x ^ (x >> 7);
    x = x & 32767;
    return x;
}

int run_one(int seed, int rounds) {
    int x = seed;
    int i = 0;

    while (i < rounds) {
        x = mix(x + i);

        if ((x & 1) != 0) {
            x = (x * 3 + 1) & 32767;
        } else {
            x = x >> 1;
        }

        i = i + 1;
    }

    return x;
}

int main() {
    int total = 0;
    int i = 0;

    while (i < 8) {
        total = total + run_one(1 + i * 97, 2000000);
        i = i + 1;
    }

    return total & 255;
}

