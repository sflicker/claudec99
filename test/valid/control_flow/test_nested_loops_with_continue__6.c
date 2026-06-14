/* test nested loops with a continue 
   expected answer is 6 */
int main() {
    int count = 0;
    for (int i=0;i<3;i+=1) {
        for (int j=0;j<3;j+=1) {
            if (j == 1) {
                continue;
            }
            count += 1;
        }
    }
    return count;    // expected: skip j==1 -> 2 inner x 3 outer = 6
}

