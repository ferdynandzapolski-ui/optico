#include "goirrt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    go_grade_t gs, gd;
    char* src = __go_malloc(10, &gs);
    char* dst = __go_malloc(10, &gd);

    strcpy(src, "hello");
    printf("Memcpy from %p to %p...\n", src, dst);
    __go_memcpy(dst, gd, src, gs, 6, 0);

    printf("Result: %s\n", dst);

    printf("Memmove with overlap...\n");
    __go_memmove(dst + 1, gd, dst, gd, 5, 0);

    printf("Result: %s\n", dst);

    printf("Memset...\n");
    __go_memset(dst, gd, 'A', 5);
    dst[5] = '\0';
    printf("Result: %s\n", dst);

    __go_free(src, gs);
    __go_free(dst, gd);
    return 0;
}
