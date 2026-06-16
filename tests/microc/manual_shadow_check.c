#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "goirrt.h"

int main() {
    int* p = (int*)malloc(sizeof(int));
    go_grade_t g = { .base = (uintptr_t)p, .end = (uintptr_t)p + sizeof(int), .perms = 0x3 };

    printf("Storing metadata for slot %p\n", &p);
    __go_shadow_store(&p, g);

    printf("Loading metadata for slot %p\n", &p);
    go_grade_t loaded = __go_shadow_load(&p);

    printf("Loaded: base=0x%lx, end=0x%lx, perms=0x%x\n", loaded.base, loaded.end, loaded.perms);

    assert(loaded.base == g.base);
    assert(loaded.end == g.end);
    assert(loaded.perms == g.perms);

    printf("Shadow manual check passed!\n");

    free(p);
    return 0;
}
