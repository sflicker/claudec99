/* Stage 80: ++ on a const-qualified lvalue is not modifiable and must be
 * rejected. */
int main(void) {
    const int x = 1;
    x++;
    return x;
}
