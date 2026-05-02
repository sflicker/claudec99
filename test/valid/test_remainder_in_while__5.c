int main() {
    int a = 7;
    int b = 2;
    int rtn = 0;
    while (a % b) {
        rtn++;
        b++;
    }
    return rtn;
}
