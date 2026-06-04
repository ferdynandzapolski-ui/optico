#include <stdio.h>
#include <stdlib.h>
#include "goirrt.h"

int main() {
    go_grade_t g;
    int *p = (int*)__go_malloc(5 * sizeof(int), &g);
    printf("Reading p[5] (OOB) with manual check...\n");
    __go_check_load(&p[5], g, sizeof(int));
    int val = p[5];
    printf("Value: %d\n", val);
    __go_free(p, g);
    return 0;
}
