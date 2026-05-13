/* Stage 40: signed/unsigned comparison via UAC — spec test case.
 * -1 as unsigned int is UINT_MAX > 1, so s < u is false. */
int main() {
    int s = -1;
    unsigned int u = 1;
    if (s < u)
        return 1;
    return 42;
}
