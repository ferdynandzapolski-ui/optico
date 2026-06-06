#include <stdio.h>
#include <stdlib.h>
#include "goirrt.h"

int main() {
    go_grade_t g;
    int *arr = (int*)__go_malloc(5 * sizeof(int), &g);
    for (int i=0; i<5; ++i) arr[i] = i+1;

    printf("Reading arr[4] (Safe)...\n");
    __go_check_load(&arr[4], g, sizeof(int));
    printf("Value: %d\n", arr[4]);

    printf("Reading arr[5] (OOB)...\n");
    __go_check_load(&arr[5], g, sizeof(int));
    printf("Value: %d\n", arr[5]);

    __go_free(arr, g);
    return 0;
}
