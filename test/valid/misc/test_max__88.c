int max3(int a, int b, int c) {

    int max = a;

    if (b > max) max = b;
    if (c > max) max = c;

    return max;
}

int main() {
    return max3(42, 17, 88);	// expected: 88
}

