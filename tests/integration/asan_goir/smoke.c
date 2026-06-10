#include <stdio.h>
#include <stdlib.h>

int main() {
    int *p = malloc(10 * sizeof(int));
    p[5] = 42;
    printf("Value: %d\n", p[5]);
    free(p);
    printf("Success!\n");
    return 0;
}
