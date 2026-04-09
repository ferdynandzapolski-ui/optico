#include <stdio.h>
#include <stdlib.h>

int main() {
    int *p = (int*)malloc(10 * sizeof(int));
    int *q = &p[5];
    printf("Freeing p, then accessing interior q...\n");
    free(p);
    int val = *q; // UAF
    printf("Value: %d\n", val);
    return 0;
}
