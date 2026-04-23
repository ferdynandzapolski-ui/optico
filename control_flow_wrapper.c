#include <stdio.h>

// Function to print integers
void print_int(int x) {
    printf("%d\n", x);
}

// External functions from OptiCo
void test_control_flow();

int main() {
    printf("Testing control flow:\n");
    test_control_flow();
    printf("\n");
    return 0;
}
