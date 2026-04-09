#include <stdio.h>
#include <string.h>

struct Data {
    int a;
    int b;
};

int main() {
    struct Data src = {1, 2};
    struct Data dst = {0, 0};
    printf("Memcpy of struct {int, int}...\n");
    memcpy(&dst, &src, sizeof(struct Data));
    if (dst.a == 1 && dst.b == 2) {
        printf("Success\n");
        return 0;
    }
    return 1;
}
