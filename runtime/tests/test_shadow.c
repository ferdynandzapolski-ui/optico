#include "go_shadow.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void test_basic_store_load() {
    printf("Testing basic store/load...\n");
    uint64_t dummy[2];
    void* ptr = (void*)(((uintptr_t)&dummy[0] + 7) & ~7UL);

    go_grade_t g = {0};
    g.base = 0x1000;
    g.end = 0x2000;
    g.perms = 0x7;

    __go_shadow_store(ptr, g);
    go_grade_t loaded = __go_shadow_load(ptr);

    assert(loaded.base == g.base);
    assert(loaded.end == g.end);
    assert(loaded.perms == g.perms);
    printf("Basic store/load passed.\n");
}

void test_unaligned_access() {
    printf("Testing unaligned access...\n");
    char buffer[16];
    void* unaligned = (void*)(((uintptr_t)buffer | 1)); // Force bit 0

    go_grade_t g = {0};
    g.base = 0x3000;

    __go_shadow_store(unaligned, g);
    go_grade_t loaded = __go_shadow_load(unaligned);

    // Should return TOP grade because it's unaligned
    assert(loaded.end == -1ULL);
    assert(loaded.perms == 0xF);
    printf("Unaligned access passed.\n");
}

void test_uninitialized_access() {
    printf("Testing uninitialized access...\n");
    uint64_t dummy[2];
    void* ptr = (void*)(((uintptr_t)&dummy[0] + 7) & ~7UL);

    // Clear shadow for this area first to be sure
    go_grade_t zero = {0};
    __go_shadow_store(ptr, zero);

    go_grade_t loaded = __go_shadow_load(ptr);
    assert(loaded.end == -1ULL);
    assert(loaded.perms == 0xF);
    printf("Uninitialized access passed.\n");
}

void test_multi_level_allocation() {
    printf("Testing multi-level allocation...\n");
    // Use addresses far apart to force different secondary tables
    // 48-bit address space.
    void* ptr1 = (void*)0x100000000000ULL; // Bit 44 set
    void* ptr2 = (void*)0x200000000000ULL; // Bit 45 set

    go_grade_t g1 = {0}; g1.base = 1;
    go_grade_t g2 = {0}; g2.base = 2;

    __go_shadow_store(ptr1, g1);
    __go_shadow_store(ptr2, g2);

    go_grade_t l1 = __go_shadow_load(ptr1);
    go_grade_t l2 = __go_shadow_load(ptr2);

    assert(l1.base == 1);
    assert(l2.base == 2);
    printf("Multi-level allocation passed.\n");
}

int main() {
    __go_shadow_init();
    test_basic_store_load();
    test_unaligned_access();
    test_uninitialized_access();
    test_multi_level_allocation();
    printf("All shadow tests passed!\n");
    return 0;
}
