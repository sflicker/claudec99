#include <string.h>
int check(void) {
    return strcmp(__func__, "check");
}
int main(void) { return check(); }
