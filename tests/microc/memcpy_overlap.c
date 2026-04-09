#include <stdio.h>
#include <string.h>

int main() {
    int arr[5] = {1, 2, 3, 4, 5};
    printf("Memmove overlapping...\n");
    memmove(&arr[1], &arr[0], 4 * sizeof(int));
    if (arr[1] == 1 && arr[2] == 2) {
        printf("Success\n");
        return 0;
    }
    return 1;
}
