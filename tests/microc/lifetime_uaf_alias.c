#include <stdio.h>
#include <stdlib.h>

int main() {
    int *p = (int*)malloc(sizeof(int));
    int *q = p;
    free(p);
    printf("Accessing alias q after p is freed (UAF)...\n");
    int val = *q; // UAF
    printf("Value: %d\n", val);
    return 0;
}
