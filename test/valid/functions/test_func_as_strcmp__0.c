#include <string.h>
static int check(void) { return strcmp(__func__, "check"); }
int main(void) { return check(); }
