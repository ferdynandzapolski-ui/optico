#include <stdio.h>

// External functions from OptiCo
void test_expressions();

void print_int(int x) {
    printf("%d ", x);
}

int main() {
    printf("Testing expressions:\n");
    test_expressions();
    printf("\n");
    return 0;
}
