#include <stdio.h>

// External functions from OptiCo
void print_int(int x) {
    printf("%d\n", x);
}
void test_functions();

int main() {
    printf("Testing functions:\n");
    test_functions();
    printf("\n");
    return 0;
}
