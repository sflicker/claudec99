#include <string.h>
int main(void) {
    char buf[8] = "abcde";
    memmove(buf + 1, buf, 4);
    return (buf[0]=='a' && buf[1]=='a' && buf[2]=='b' && buf[3]=='c' && buf[4]=='d') ? 0 : 1;
}
