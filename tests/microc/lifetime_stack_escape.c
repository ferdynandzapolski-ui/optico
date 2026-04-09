#include <stdio.h>
#include <stdlib.h>

int* get_stack_ptr() {
    int x = 42;
    return &x; // Returning stack address
}

int main() {
    int *p = get_stack_ptr();
    printf("Accessing stack pointer after function return...\n");
    int val = *p; // Stack UAF
    printf("Value: %d\n", val);
    return 0;
}
