/*
 * This is a multi-line block comment.
 * It spans several lines and should be
 * skipped entirely by the tokenizer.
 */
int main() {
    int x = 40;
    /* another
       multi-line
       block */
    x = x + 2; // trailing line comment
    return x;
}
