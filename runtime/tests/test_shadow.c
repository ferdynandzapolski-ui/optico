#include "go_shadow.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void test_basic_shadow() {
    printf("Running test_basic_shadow...\n");
    void* p = (void*)0x12345678000;
    go_grade_t g = {0};
    g.base = 0x1000;
    g.end = 0x2000;
    g.perms = 0x7;

    __go_shadow_store(p, g);
    go_grade_t g_loaded = __go_shadow_load(p);

    assert(g_loaded.base == g.base);
    assert(g_loaded.end == g.end);
    assert(g_loaded.perms == g.perms);
    printf("test_basic_shadow passed.\n");
}

void test_unaligned_shadow() {
    printf("Running test_unaligned_shadow...\n");
    void* p = (void*)0x12345678001; // Unaligned
    go_grade_t g = {0};
    g.base = 0x3000;
    g.end = 0x4000;

    __go_shadow_store(p, g);
    go_grade_t g_loaded = __go_shadow_load(p);

    // Should return TOP grade
    assert(g_loaded.end == -1ULL);
    assert(g_loaded.perms == 0xF);
    printf("test_unaligned_shadow passed.\n");
}

void test_large_address_space() {
    printf("Running test_large_address_space...\n");
    // Test mapping at the edge of 48-bit address space
    void* p1 = (void*)0x8000; // Low address
    void* p2 = (void*)0xFFFFFFFFF8; // High address (40 bits shown here, but let's go higher)
    void* p3 = (void*)0x7FFFFFFFFF8; // ~47 bits

    go_grade_t g1 = { .base = 1, .end = 2 };
    go_grade_t g2 = { .base = 3, .end = 4 };
    go_grade_t g3 = { .base = 5, .end = 6 };

    __go_shadow_store(p1, g1);
    __go_shadow_store(p2, g2);
    __go_shadow_store(p3, g3);

    go_grade_t l1 = __go_shadow_load(p1);
    go_grade_t l2 = __go_shadow_load(p2);
    go_grade_t l3 = __go_shadow_load(p3);

    assert(l1.base == 1);
    assert(l2.base == 3);
    assert(l3.base == 5);
    printf("test_large_address_space passed.\n");
}

void test_overlapping_slots() {
    printf("Running test_overlapping_slots...\n");
    void* p1 = (void*)0x2000;
    void* p2 = (void*)0x2008;

    go_grade_t g1 = { .base = 100 };
    go_grade_t g2 = { .base = 200 };

    __go_shadow_store(p1, g1);
    __go_shadow_store(p2, g2);

    assert(__go_shadow_load(p1).base == 100);
    assert(__go_shadow_load(p2).base == 200);
    printf("test_overlapping_slots passed.\n");
}

int main() {
    __go_shadow_init();
    test_basic_shadow();
    test_unaligned_shadow();
    test_large_address_space();
    test_overlapping_slots();
    printf("All shadow tests passed!\n");
    return 0;
}
