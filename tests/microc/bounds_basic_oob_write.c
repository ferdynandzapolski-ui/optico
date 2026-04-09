#include <stdio.h>
#include <stdlib.h>

int main() {
    int arr[5] = {1, 2, 3, 4, 5};
    int *p = arr;
    printf("Writing arr[5] (OOB)...\n");
    p[5] = 42; // OOB Write
    printf("Write completed\n");
    return 0;
}
