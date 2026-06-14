/* Test for a Break in a for loop */
int main() {
    int sum = 0;
    for (int i = 0;i < 10; i += 1) {
        if (i == 5) {
			break;
        }
        sum += i;
    }
    return sum;      // expected 0+1+2+3+4 = 10
}

