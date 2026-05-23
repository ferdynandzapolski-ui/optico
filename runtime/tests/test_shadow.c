#include "goirrt.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void test_basic_store_load() {
    printf("Testing basic shadow store/load...\n");
    int x;
    void* p = &x;
    // Align to 8 bytes for the test
    p = (void*)(((uintptr_t)p + 7) & ~7);

    go_grade_t g = {0};
    g.base = 0x1000;
    g.end = 0x2000;
    g.perms = 0x3;

    __go_shadow_store(p, g);
    go_grade_t g_loaded = __go_shadow_load(p);

    assert(g_loaded.base == g.base);
    assert(g_loaded.end == g.end);
    assert(g_loaded.perms == g.perms);
    printf("Basic shadow store/load passed.\n");
}

void test_top_grade_default() {
    printf("Testing default TOP grade...\n");
    int y;
    void* p = &y;
    p = (void*)(((uintptr_t)p + 7) & ~7);

    // Use an address far away from any previous store
    void* p_remote = (void*)((uintptr_t)p + (1ULL << 30));

    go_grade_t g = __go_shadow_load(p_remote);
    go_grade_t top = __go_get_top_grade();

    assert(g.base == top.base);
    assert(g.end == top.end);
    assert(g.perms == top.perms);
    printf("Default TOP grade passed.\n");
}

void test_overlapping_stores() {
    printf("Testing overlapping stores (consecutive slots)...\n");
    uint64_t slots[2];
    void* p1 = &slots[0];
    void* p2 = &slots[1];

    go_grade_t g1 = {0x100, 0x200, 0, 0, 0x1, 0, 0, 0};
    go_grade_t g2 = {0x300, 0x400, 0, 0, 0x2, 0, 0, 0};

    __go_shadow_store(p1, g1);
    __go_shadow_store(p2, g2);

    go_grade_t r1 = __go_shadow_load(p1);
    go_grade_t r2 = __go_shadow_load(p2);

    assert(r1.base == g1.base && r1.perms == g1.perms);
    assert(r2.base == g2.base && r2.perms == g2.perms);
    printf("Overlapping stores passed.\n");
}

int main() {
    test_basic_store_load();
    test_top_grade_default();
    test_overlapping_stores();
    printf("All shadow store unit tests passed!\n");
    return 0;
}
