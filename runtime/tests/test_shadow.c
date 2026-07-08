#include "go_shadow.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

void test_basic_store_load() {
    printf("Testing basic store/load...\n");
    uint64_t slot_val = 0x12345678;
    void* slot_ptr = &slot_val;

    go_grade_t g = {0};
    g.base = 0x1000;
    g.end = 0x2000;
    g.perms = 0x3;

    __go_shadow_store(slot_ptr, g);

    go_grade_t loaded_g = __go_shadow_load(slot_ptr);

    assert(loaded_g.base == g.base);
    assert(loaded_g.end == g.end);
    assert(loaded_g.perms == g.perms);
    printf("Basic store/load passed!\n");
}

void test_unaligned_access() {
    printf("Testing unaligned access...\n");
    char buffer[16];
    void* unaligned_ptr = &buffer[1];

    go_grade_t g = {0};
    g.base = 0x3000;

    // Store should be ignored for unaligned address
    __go_shadow_store(unaligned_ptr, g);

    go_grade_t loaded_g = __go_shadow_load(unaligned_ptr);

    // Should return TOP grade (end == -1ULL)
    assert(loaded_g.end == -1ULL);
    printf("Unaligned access test passed!\n");
}

void test_multi_level_allocation() {
    printf("Testing multi-level allocation...\n");
    // Access distant addresses to trigger secondary table allocation
    void* ptr1 = (void*)0x100000000ULL;
    void* ptr2 = (void*)0x200000000ULL;

    go_grade_t g1 = {0}; g1.base = 0x1;
    go_grade_t g2 = {0}; g2.base = 0x2;

    __go_shadow_store(ptr1, g1);
    __go_shadow_store(ptr2, g2);

    assert(__go_shadow_load(ptr1).base == 0x1);
    assert(__go_shadow_load(ptr2).base == 0x2);
    printf("Multi-level allocation test passed!\n");
}

int main() {
    test_basic_store_load();
    test_unaligned_access();
    test_multi_level_allocation();
    printf("All shadow tests passed!\n");
    return 0;
}
