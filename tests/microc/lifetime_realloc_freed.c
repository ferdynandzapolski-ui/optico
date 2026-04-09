#include <stdio.h>
#include <stdlib.h>

int main() {
    int *p = (int*)malloc(sizeof(int));
    printf("Freeing p...\n");
    free(p);
    printf("Using realloc on freed p (UAF)...\n");
    int *q = (int*)realloc(p, 10 * sizeof(int)); // UAF
    if (q) free(q);
    return 0;
}
