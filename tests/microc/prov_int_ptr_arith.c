#include <stdio.h>
#include <stdint.h>

int main() {
    int arr[2] = {1, 2};
    int *p = &arr[0];
    uintptr_t i = (uintptr_t)p;
    i += sizeof(int);
    int *q = (int*)i;
    printf("Accessing q after ptr->int+offset->ptr...\n");
    int val = *q;
    printf("Value: %d (expected 2)\n", val);
    return 0;
}
