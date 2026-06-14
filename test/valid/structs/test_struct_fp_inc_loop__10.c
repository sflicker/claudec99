struct Acc { double sum; };

int main(void) {
    struct Acc a;
    a.sum = 0.0;
    int i;
    for (i = 0; i < 10; i++)
        ++a.sum;
    return (int)a.sum;   /* 10 */
}
