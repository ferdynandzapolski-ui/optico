#include "goirrt.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    go_grade_t gs, gd;
    char* src = __go_malloc(10, &gs);
    char* dst = __go_malloc(10, &gd);

    printf("Memcpy OOB src...\n");
    __go_memcpy(dst, gd, src, gs, 11, 0);

    return 0;
}
