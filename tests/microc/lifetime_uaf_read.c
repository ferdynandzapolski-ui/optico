#include <stdio.h>
#include <stdlib.h>

int main() {
    int *p = (int*)malloc(sizeof(int));
    *p = 42;
    free(p);
    printf("Accessing p after free (UAF)...\n");
    int val = *p; // UAF
    printf("Value: %d\n", val);
    return 0;
}
