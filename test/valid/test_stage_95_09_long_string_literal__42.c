/* Stage 95-09: verify that a string literal longer than the old MAX_NAME_LEN
 * (255 bytes) is handled correctly now that ASTNode.value is a pointer rather
 * than a fixed char[256] buffer. */
int strlen(char *s);

int main(void) {
    /* 260-character string — exceeds old 255-byte ASTNode.value limit */
    char *s = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    if (strlen(s) == 260) return 42;
    return 1;
}
