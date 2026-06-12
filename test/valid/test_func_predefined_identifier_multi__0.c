#include <string.h>
int foo(void) { return strcmp(__func__, "foo"); }
int bar(void) { return strcmp(__func__, "bar"); }
int main(void) { return foo() + bar(); }
