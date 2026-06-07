#include "goirrt.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main() {
    go_grade_t g1 = {0x1000, 0x2000, 1, 1, 0xF, 0, 0, 0};
    void* slot = (void*)0x7fffffff0000;

    printf("Storing grade to shadow at %p...\n", slot);
    __go_shadow_store(slot, g1);

    printf("Loading grade from shadow...\n");
    go_grade_t g2 = __go_shadow_load(slot);

    printf("Loaded: base=%lx, end=%lx, perms=%x\n", g2.base, g2.end, g2.perms);
    assert(g2.base == g1.base);
    assert(g2.end == g1.end);
    assert(g2.perms == g1.perms);

    printf("Shadow check passed\n");
    return 0;
}
