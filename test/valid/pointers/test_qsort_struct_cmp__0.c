#include <stdlib.h>
#include <string.h>

struct Item { int key; int val; };

static int item_cmp(const void *a, const void *b) {
    const struct Item *ia = (const struct Item *)a;
    const struct Item *ib = (const struct Item *)b;
    return ia->key - ib->key;   /* uses arrow access (rbx scratch) */
}

int main(void) {
    struct Item items[4];
    items[0].key = 30; items[0].val = 3;
    items[1].key = 10; items[1].val = 1;
    items[2].key = 40; items[2].val = 4;
    items[3].key = 20; items[3].val = 2;

    qsort(items, 4, sizeof(struct Item), item_cmp);

    /* After sort: keys should be 10, 20, 30, 40 */
    if (items[0].key != 10) return 1;
    if (items[1].key != 20) return 2;
    if (items[2].key != 30) return 3;
    if (items[3].key != 40) return 4;
    return 0;
}
