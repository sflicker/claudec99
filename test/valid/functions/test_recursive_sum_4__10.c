int sum_up(int n) {
    if (n == 0) {
        return 0;
    }
    else {
    	return n + sum_up(n - 1);
    }
}

int main() {
    return sum_up(4);
}

