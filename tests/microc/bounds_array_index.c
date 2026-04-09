#include <stdio.h>
#include <stdlib.h>

int main() {
    int arr[5] = {1, 2, 3, 4, 5};
    int idx = 10;
    printf("Accessing arr[%d]...\n", idx);
    int val = arr[idx]; // OOB
    printf("Value: %d\n", val);
    return 0;
}
