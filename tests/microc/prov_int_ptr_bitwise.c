#include <stdio.h>
#include <stdint.h>

int main() {
    int x = 42;
    uintptr_t i = (uintptr_t)&x;
    uintptr_t mask = 0xFFFFFFFFFFFFFFFF;
    uintptr_t j = i & mask;
    int *q = (int*)j;
    printf("Accessing q after bitwise AND on ptr-as-int...\n");
    int val = *q;
    printf("Value: %d\n", val);
    return 0;
}
