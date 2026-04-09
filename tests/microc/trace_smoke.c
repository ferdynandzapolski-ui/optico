#include <stdio.h>
#include <stdlib.h>
#include "goirrt.h"

int main() {
    go_grade_t g;
    int* p = (int*)__go_malloc(sizeof(int) * 10, &g);

    printf("Allocated at %p\n", p);

    // In-bounds load
    __go_check_load(p, g, sizeof(int));

    // Out-of-bounds load
    __go_check_load(p + 10, g, sizeof(int));

    __go_free(p, g);
    return 0;
}
