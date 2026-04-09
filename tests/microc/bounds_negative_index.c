#include <stdio.h>
#include <stdlib.h>

int main() {
    int arr[5] = {1, 2, 3, 4, 5};
    int *p = arr;
    printf("Accessing p[-1]...\n");
    int val = p[-1]; // OOB
    printf("Value: %d\n", val);
    return 0;
}
