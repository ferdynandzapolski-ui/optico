#include <stdio.h>

void print_int(int val) {
    printf("%d\n", val);
}

extern void optico_main();

int main() {
    optico_main();
    return 0;
}
