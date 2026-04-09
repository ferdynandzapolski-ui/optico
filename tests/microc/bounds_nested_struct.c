#include <stdio.h>

struct Inner {
    int x[2];
};

struct Outer {
    struct Inner in;
    int y;
};

int main() {
    struct Outer obj = {{{1, 2}}, 3};
    int *p = &obj.in.x[2]; // OOB for Inner.x
    printf("Accessing obj.in.x[2]...\n");
    int val = *p;
    printf("Value: %d\n", val);
    return 0;
}
