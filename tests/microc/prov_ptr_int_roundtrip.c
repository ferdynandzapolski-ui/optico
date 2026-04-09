#include <stdio.h>
#include <stdint.h>

int main() {
    int x = 42;
    int *p = &x;
    uintptr_t i = (uintptr_t)p;
    int *q = (int*)i;
    printf("Roundtrip p -> i -> q: %p == %p\n", (void*)p, (void*)q);
    if (p == q && *q == 42) {
        printf("Success\n");
        return 0;
    }
    return 1;
}
