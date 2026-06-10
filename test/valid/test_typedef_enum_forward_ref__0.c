typedef enum Status Status;
enum Status { OK = 0, ERR = 1 };
Status check(int v) { return v ? ERR : OK; }
int main() { return check(1) - 1; }
