struct Acc { double sum; double cur; };
static struct Acc acc;

int main(void) {
    int i;
    acc.sum = 0.0;
    for (i = 1; i <= 5; i++) {
        acc.cur  = (double)i;
        acc.sum += acc.cur;   /* 1+2+3+4+5 = 15 */
    }
    return (int)acc.sum;
}
