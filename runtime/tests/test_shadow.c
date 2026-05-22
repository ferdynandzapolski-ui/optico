#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "go_shadow.h"

void test_basic_store_load() {
    printf("Testing basic store/load...\n");
    int* p = (int*)0x10008;
    go_grade_t g = {0};
    g.base = 0x10000;
    g.end = 0x10010;
    g.perms = 0x3;

    __go_shadow_store(p, g);
    go_grade_t loaded = __go_shadow_load(p);

    assert(loaded.base == g.base);
    assert(loaded.end == g.end);
    assert(loaded.perms == g.perms);
    printf("Basic store/load passed.\n");
}

void test_top_grade_default() {
    printf("Testing default TOP grade for unmapped address...\n");
    int* p = (int*)0x20008;
    go_grade_t loaded = __go_shadow_load(p);

    assert(loaded.end == -1ULL);
    assert(loaded.perms == 0xF);
    printf("Default TOP grade passed.\n");
}

void test_overlapping_stores() {
    printf("Testing overlapping stores (different slots)...\n");
    void* p1 = (void*)0x30008;
    void* p2 = (void*)0x30010;

    go_grade_t g1 = {0x100, 0x200, 1, 1, 0x1, 0, 0, 0};
    go_grade_t g2 = {0x300, 0x400, 2, 2, 0x2, 0, 0, 0};

    __go_shadow_store(p1, g1);
    __go_shadow_store(p2, g2);

    go_grade_t l1 = __go_shadow_load(p1);
    go_grade_t l2 = __go_shadow_load(p2);

    assert(l1.base == 0x100);
    assert(l2.base == 0x300);
    printf("Overlapping stores passed.\n");
}

int main() {
    test_basic_store_load();
    test_top_grade_default();
    test_overlapping_stores();
    printf("All shadow tests passed!\n");
    return 0;
}
