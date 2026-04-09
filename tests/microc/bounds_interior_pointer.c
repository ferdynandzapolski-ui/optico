#include <stdio.h>
#include <stdlib.h>

int main() {
    int arr[5] = {1, 2, 3, 4, 5};
    int *p = &arr[2]; // Legal interior pointer
    printf("Interior pointer value: %d\n", *p);
    if (*p == 3) {
        printf("Interior pointer test passed\n");
        return 0;
    }
    return 1;
}
