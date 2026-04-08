#include <stdio.h>

int main() {
    int a = 10;
    int b = 20;
    int c = a + b;
    if (c == 30) {
        printf("Arithmetic passed: %d + %d = %d\n", a, b, c);
        return 0;
    }
    return 1;
}
