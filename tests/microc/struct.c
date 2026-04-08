#include <stdio.h>

struct Point {
    int x;
    int y;
};

int main() {
    struct Point p = {10, 20};
    if (p.x == 10 && p.y == 20) {
        printf("Struct passed: p.x=%d, p.y=%d\n", p.x, p.y);
        return 0;
    }
    return 1;
}
