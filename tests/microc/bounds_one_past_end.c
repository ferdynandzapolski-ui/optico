#include <stdio.h>
#include <stdlib.h>

int main() {
    int arr[5] = {1, 2, 3, 4, 5};
    int *p = &arr[5]; // Legal one-past-the-end
    printf("Pointer at one-past-the-end: %p\n", (void*)p);
    printf("Attempting to dereference one-past-the-end...\n");
    int val = *p; // Illegal dereference
    printf("Value: %d\n", val);
    return 0;
}
