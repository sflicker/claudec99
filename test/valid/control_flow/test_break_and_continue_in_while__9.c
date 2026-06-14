/* test break and continue in a while loop.
   expected answer is 9.
*/

int main() {
    int i = 0;
    int sum = 0;
    while (i < 10) {
        i += 1;
        if (i == 7) {
            break;
        }
        if (i % 2 == 0) {
            continue;
        }
        sum += i;
    }
    return sum;  // Expected: 1+3+5 = 9 (stops before 7)
}
