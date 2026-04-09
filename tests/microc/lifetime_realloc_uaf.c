#include <stdio.h>
#include <stdlib.h>

int main() {
    int *p = (int*)malloc(10 * sizeof(int));
    printf("Original p: %p\n", (void*)p);
    int *q = (int*)realloc(p, 1000 * sizeof(int)); // May move
    printf("New q: %p\n", (void*)q);
    if (p != q) {
        printf("Accessing old p after realloc (UAF)...\n");
        int val = *p; // UAF
        printf("Value: %d\n", val);
    } else {
        printf("Realloc didn't move, skipping UAF check for old pointer\n");
    }
    free(q);
    return 0;
}
