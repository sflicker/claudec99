#include <ctype.h>
int main(void) {
    if (!isprint('A'))  return 1;
    if (!ispunct('!'))  return 2;
    if (!isgraph('A'))  return 3;
    if ( iscntrl('A'))  return 4;
    if (!iscntrl('\t')) return 5;
    return 0;
}
