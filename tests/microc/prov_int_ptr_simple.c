#include <stdio.h>
#include <stdint.h>

int main() {
    int x = 42;
    int *p = &x;
    uintptr_t i = (uintptr_t)p;
    i += 0; // No-op
    int *q = (int*)i;
    printf("Accessing q after ptr->int->ptr (no offset)...\n");
    int val = *q;
    printf("Value: %d\n", val);
    return 0;
}
