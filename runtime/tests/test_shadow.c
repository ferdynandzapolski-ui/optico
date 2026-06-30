#include "go_shadow.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

void test_basic_store_load() {
    printf("Testing basic store/load...\n");
    void* ptr = (void*)0x12345678000ULL; // 8-byte aligned
    go_grade_t g = { .base = 0x1000, .end = 0x2000, .perms = 0x1 };

    __go_shadow_store(ptr, g);
    go_grade_t loaded = __go_shadow_load(ptr);

    assert(loaded.base == g.base);
    assert(loaded.end == g.end);
    assert(loaded.perms == g.perms);
    printf("Basic store/load passed.\n");
}

void test_top_fallback() {
    printf("Testing TOP fallback...\n");
    void* ptr = (void*)0x87654321000ULL;
    go_grade_t loaded = __go_shadow_load(ptr);

    // Default TOP for uninitialized: base=0, end=-1ULL, perms=0xF
    assert(loaded.base == 0);
    assert(loaded.end == -1ULL);
    assert(loaded.perms == 0xF);
    printf("TOP fallback passed.\n");
}

void test_unaligned_access() {
    printf("Testing unaligned access...\n");
    void* ptr = (void*)0x12345678001ULL; // Not 8-byte aligned
    go_grade_t g = { .base = 0x1000, .end = 0x2000, .perms = 0x1 };

    __go_shadow_store(ptr, g);
    go_grade_t loaded = __go_shadow_load(ptr);

    // Should return TOP since store should have been ignored and load should return TOP for unaligned
    assert(loaded.base == 0);
    assert(loaded.end == -1ULL);
    assert(loaded.perms == 0xF);
    printf("Unaligned access passed.\n");
}

void test_multi_level_allocation() {
    printf("Testing multi-level allocation...\n");
    // Use addresses that fall into different primary/secondary indices
    void* ptr1 = (void*)0x10000000000ULL;
    void* ptr2 = (void*)0x20000000000ULL;

    go_grade_t g1 = { .base = 0x1111, .end = 0x2222, .perms = 0x2 };
    go_grade_t g2 = { .base = 0x3333, .end = 0x4444, .perms = 0x4 };

    __go_shadow_store(ptr1, g1);
    __go_shadow_store(ptr2, g2);

    go_grade_t l1 = __go_shadow_load(ptr1);
    go_grade_t l2 = __go_shadow_load(ptr2);

    assert(l1.base == g1.base);
    assert(l2.base == g2.base);
    printf("Multi-level allocation passed.\n");
}

int main() {
    test_basic_store_load();
    test_top_fallback();
    test_unaligned_access();
    test_multi_level_allocation();
    printf("All shadow store tests passed!\n");
    return 0;
}
