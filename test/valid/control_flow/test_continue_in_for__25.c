/* Test continue in a for loop
   Expected answer is 25. */

int main() {
    int sum = 0;
    for (int i = 0;i < 10;i+=1) {
        if (i % 2 == 0) {
            continue;    // don't sum even numbers
        }
        sum += i;
    }
    return sum;
}

