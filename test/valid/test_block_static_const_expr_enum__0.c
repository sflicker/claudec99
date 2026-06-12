enum { BASE = 10 };
int main(void) {
    static int limit = BASE * 2 + 5;
    return limit - 25;   /* expect 0 */
}
