#include <stdio.h>
#include <stdlib.h>

int main() {
    int *p = (int*)malloc(sizeof(int));
    free(p);
    printf("Writing p after free (UAF)...\n");
    *p = 42; // UAF
    printf("Write completed\n");
    return 0;
}
