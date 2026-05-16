typedef unsigned long size_t;
void *malloc(size_t size);
void free(void *);

int main() {
    int *block = malloc(256);
    free(block);
    return 0;
}
