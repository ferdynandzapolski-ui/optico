#include <stdio.h>
#include <stdlib.h>

int main() {
    int arr[5] = {1, 2, 3, 4, 5};
    int *p = arr;
    printf("Reading arr[5] (OOB)...\n");
    int val = p[5]; // OOB Read
    printf("Value: %d\n", val);
    return 0;
}
