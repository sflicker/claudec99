/* CC99-004: global and static pointer initializers from address constants */
int x = 37;
int *p = &x;

int main(void) {
    return *p;
}
