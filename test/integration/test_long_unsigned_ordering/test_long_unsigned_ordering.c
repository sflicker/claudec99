/* C99 allows any ordering of type specifiers: long unsigned == unsigned long */
typedef long unsigned int size_t_alias;
typedef long unsigned     ulong_alias;
typedef long signed int   long_alias;

int main(void) {
    size_t_alias a = 10;
    ulong_alias  b = 20;
    long_alias   c = -5;
    return (int)(a + b + (size_t_alias)c) == 25 ? 0 : 1;
}
