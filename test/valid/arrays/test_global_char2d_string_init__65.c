char words[3][8] = {"hello", "world", "A"};
int main(void) {
    /* 'h'=104, 'w'=119, 'A'=65 — return first char of third string */
    return (int)(unsigned char)words[2][0];
}
