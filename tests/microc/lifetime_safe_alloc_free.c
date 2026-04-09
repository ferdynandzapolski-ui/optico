#include <stdio.h>
#include <stdlib.h>

int main() {
    int *p = (int*)malloc(sizeof(int));
    *p = 42;
    printf("Normal access: %d\n", *p);
    free(p);
    printf("Success\n");
    return 0;
}
