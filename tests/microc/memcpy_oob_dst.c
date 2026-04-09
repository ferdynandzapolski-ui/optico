#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main() {
    char *src = "Hello";
    char dst[10];
    printf("Memcpy with OOB destination...\n");
    // Incorrect length
    memcpy(dst, src, 20);
    printf("Completed (unexpected)\n");
    return 0;
}
