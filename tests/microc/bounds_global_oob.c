#include <stdio.h>

int global_arr[5] = {1, 2, 3, 4, 5};

int main() {
    printf("Accessing global_arr[5]...\n");
    int val = global_arr[5]; // OOB
    printf("Value: %d\n", val);
    return 0;
}
