#include "helper.h"
int strlen(const char *s);
int fileTest(void) {
    return strlen(__FILE__) > 0 ? 0 : 1;
}
int lineTest(void) {
    return __LINE__ - 7;
}
