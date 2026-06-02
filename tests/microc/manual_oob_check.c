#include <stdio.h>
#include <stdlib.h>
#include "goirrt.h"

int main() {
    go_grade_t g_p;
    int *p = (int*)__go_malloc(5 * sizeof(int), &g_p);
    for(int i=0; i<5; i++) p[i] = i+1;

    printf("Reading p[5] (OOB)...\n");
    __go_check_load(p + 5, g_p, sizeof(int));
    int val = p[5]; // OOB Read
    printf("Value: %d\n", val);

    __go_free(p, g_p);
    return 0;
}
