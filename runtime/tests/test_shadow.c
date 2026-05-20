#include "go_shadow.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void test_basic_store_load() {
    printf("Testing basic store/load...\n");
    int x;
    void* slot = &x;
    // Align to 8 bytes if needed
    slot = (void*)(((uintptr_t)slot >> 3) << 3);

    go_grade_t g = {0};
    g.base = 0x1000;
    g.end = 0x2000;
    g.perms = 0x3;

    __go_shadow_store(slot, g);
    go_grade_t g_loaded = __go_shadow_load(slot);

    assert(g_loaded.base == g.base);
    assert(g_loaded.end == g.end);
    assert(g_loaded.perms == g.perms);
    printf("PASSED\n");
}

void test_default_top() {
    printf("Testing default TOP grade for uninitialized slots...\n");
    // Some random address that hasn't been used
    void* slot = (void*)0x7FFF00001000ULL;

    go_grade_t g_loaded = __go_shadow_load(slot);

    assert(g_loaded.base == 0);
    assert(g_loaded.end == -1ULL);
    assert(g_loaded.perms == 0xF);
    printf("PASSED\n");
}

void test_wide_range() {
    printf("Testing store/load across wide address range...\n");
    void* slot1 = (void*)0x10000000ULL;
    void* slot2 = (void*)0x700000000000ULL;

    go_grade_t g1 = { .base = 0x1, .end = 0x2 };
    go_grade_t g2 = { .base = 0x3, .end = 0x4 };

    __go_shadow_store(slot1, g1);
    __go_shadow_store(slot2, g2);

    go_grade_t r1 = __go_shadow_load(slot1);
    go_grade_t r2 = __go_shadow_load(slot2);

    assert(r1.base == g1.base);
    assert(r2.base == g2.base);
    printf("PASSED\n");
}

int main() {
    test_basic_store_load();
    test_default_top();
    test_wide_range();
    printf("All shadow tests passed!\n");
    return 0;
}
