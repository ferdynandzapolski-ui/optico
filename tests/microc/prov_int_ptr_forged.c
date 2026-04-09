#include <stdio.h>
#include <stdint.h>

int main() {
    int x = 42;
    uintptr_t i = (uintptr_t)&x;
    // Attempting to forge a pointer from a literal (illegal in strict provenance)
    int *q = (int*)i;
    printf("Dereferencing forged-like pointer...\n");
    int val = *q;
    printf("Value: %d\n", val);
    return 0;
}
