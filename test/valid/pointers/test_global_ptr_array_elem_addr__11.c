/* CC99-004B: global pointer initialized from array element address */
int xs[3] = { 5, 7, 11 };
int *p = &xs[2];

int main(void) {
    return *p;
}
