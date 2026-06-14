struct Node { double score; };

int main(void) {
    struct Node n;
    struct Node *p = &n;
    p->score = 5.0;
    ++p->score;
    return (int)p->score;   /* 6 */
}
