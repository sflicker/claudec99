#include <stdio.h>
#include <string.h>
#include <zlib.h>

int main(void) {
    const unsigned char source[] = "Hello From ClaudeC99";
    unsigned char compressed[100];

    uLong source_size = strlen((const char *)source) + 1;
    uLong compressed_size = sizeof(compressed);

    int result = compress(
        compressed,
        &compressed_size,
        source,
        source_size
    );

    if (result != Z_OK) {
        printf("Compression failed: %d\n", result);
        return 1;
    }

    printf("Original: %lu bytes\n", (unsigned long)source_size);
    printf("Compressed: %lu bytes\n", (unsigned long)compressed_size);

    return 0;
}