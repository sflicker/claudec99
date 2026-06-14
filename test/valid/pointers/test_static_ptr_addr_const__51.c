/* CC99-004D: block-scope static pointer initialized from address constant */
int x = 51;

int main(void) {
    static int *p = &x;
    return *p;
}
