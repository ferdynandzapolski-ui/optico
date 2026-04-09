#include <stdio.h>
#include <stdlib.h>

int main() {
    int *p = (int*)malloc(sizeof(int));
    printf("First free...\n");
    free(p);
    printf("Second free (Double Free)...\n");
    free(p); // Double Free
    printf("Completed\n");
    return 0;
}
