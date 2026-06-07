#include "goirrt.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    go_grade_t g;
    int* p = __go_malloc(5 * sizeof(int), &g);
    printf("Malloced at %p, base=%lx, end=%lx\n", p, g.base, g.end);

    printf("Accessing index 0 (safe)...\n");
    __go_check_load(p, g, sizeof(int));
    p[0] = 1;

    printf("Accessing index 5 (OOB)...\n");
    __go_check_load(p + 5, g, sizeof(int));

    __go_free(p, g);
    return 0;
}
