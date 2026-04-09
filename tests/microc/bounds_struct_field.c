#include <stdio.h>
#include <stdlib.h>

struct Data {
    int a;
    int b[3];
    int c;
};

int main() {
    struct Data d = {1, {2, 3, 4}, 5};
    int *p = &d.b[3]; // OOB relative to field b, but in-bounds for struct d
    printf("Accessing d.b[3] (field OOB)...\n");
    int val = *p;
    printf("Value: %d\n", val);
    return 0;
}
