char rows[2][16] = {"hello", "world"};
int main(void) {
    return (int)(unsigned char)rows[1][0];  /* 'w' = 119 */
}
