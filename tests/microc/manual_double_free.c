#include <stdio.h>
#include <stdlib.h>
#include "goirrt.h"

int main() {
    go_grade_t g;
    int *p = (int*)__go_malloc(sizeof(int), &g);
    printf("First free...\n");
    __go_free(p, g);
    printf("Second free (Double Free)...\n");
    __go_free(p, g); // Double Free
    printf("Completed\n");
    return 0;
}
