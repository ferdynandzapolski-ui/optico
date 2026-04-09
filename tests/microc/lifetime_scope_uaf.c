#include <stdio.h>
#include <stdlib.h>

int main() {
    int *p;
    {
        int x = 42;
        p = &x;
    }
    printf("Accessing p after scope ended (UAF)...\n");
    int val = *p; // Scope UAF
    printf("Value: %d\n", val);
    return 0;
}
