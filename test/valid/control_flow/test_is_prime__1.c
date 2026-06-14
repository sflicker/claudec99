int is_prime(int n) {
    if (n <= 1) return 0;
    for (int i = 2;i<n;i += 1) {
        if (n % i == 0) return 0;
    }
    return 1;
}

int main() {
    return is_prime(29); // expected: 1
}

