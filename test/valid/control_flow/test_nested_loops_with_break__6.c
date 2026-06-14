/* Test Nested loops with a break
   expected answer is 6 */
int main() {
    int count = 0;
    for (int i=0;i<3;i+=1) {
        for (int j=0;j<3;j+=1) {
            if (j == 2) {
                break;
            }
            count += 1;
        }
    }
    return count;     // expected: 2 for each inner iters: and 3 for outer so 2x3=6.
}

