int main(void) {
    int before = __LINE__;
#line 100
    int after = __LINE__;
    return (before < 100 && after == 100) ? 0 : 1;
}
