#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main() {
    char src[5] = "abc";
    char dst[10];
    printf("Memcpy with OOB source...\n");
    // Incorrect length
    memcpy(dst, src, 10);
    printf("Completed (unexpected)\n");
    return 0;
}
