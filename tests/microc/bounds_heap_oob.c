#include <stdio.h>
#include <stdlib.h>

int main() {
    int *p = (int*)malloc(5 * sizeof(int));
    if (!p) return 1;
    for(int i=0; i<5; i++) p[i] = i;

    printf("Accessing heap p[5]...\n");
    int val = p[5]; // OOB
    printf("Value: %d\n", val);

    free(p);
    return 0;
}
