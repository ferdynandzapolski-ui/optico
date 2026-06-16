#include "go_shadow.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

void test_basic_store_load() {
    printf("Testing basic store/load...\n");
    uint64_t ptr_slot = 0x123456780000ULL;
    go_grade_t g = {0};
    g.base = 0x1000;
    g.end = 0x2000;
    g.perms = 0x3;

    __go_shadow_store((void*)ptr_slot, g);
    go_grade_t loaded = __go_shadow_load((void*)ptr_slot);

    assert(loaded.base == g.base);
    assert(loaded.end == g.end);
    assert(loaded.perms == g.perms);
    printf("  Passed!\n");
}

void test_sparse_allocation() {
    printf("Testing sparse allocation...\n");
    uint64_t slot1 = 0x100000000000ULL;
    uint64_t slot2 = 0x700000000000ULL; // Far away

    go_grade_t g1 = { .base = 0x1, .end = 0x2, .perms = 0x1 };
    go_grade_t g2 = { .base = 0xA, .end = 0xB, .perms = 0x2 };

    __go_shadow_store((void*)slot1, g1);
    __go_shadow_store((void*)slot2, g2);

    go_grade_t l1 = __go_shadow_load((void*)slot1);
    go_grade_t l2 = __go_shadow_load((void*)slot2);

    assert(l1.base == 1);
    assert(l2.base == 10);
    printf("  Passed!\n");
}

void test_alignment() {
    printf("Testing alignment handling...\n");
    uint64_t unaligned = 0x123456780004ULL;
    go_grade_t g = { .base = 0x55, .end = 0x66, .perms = 0x3 };

    __go_shadow_store((void*)unaligned, g);
    go_grade_t loaded = __go_shadow_load((void*)unaligned);

    // Should return TOP for unaligned load
    assert(loaded.end == -1ULL);
    assert(loaded.perms == 0xF);
    printf("  Passed!\n");
}

void test_overwrite() {
    printf("Testing overwrite...\n");
    uint64_t slot = 0xabcdef000ULL;
    go_grade_t g1 = { .base = 0x100 };
    go_grade_t g2 = { .base = 0x200 };

    __go_shadow_store((void*)slot, g1);
    __go_shadow_store((void*)slot, g2);

    go_grade_t loaded = __go_shadow_load((void*)slot);
    assert(loaded.base == 0x200);
    printf("  Passed!\n");
}

int main() {
    __go_shadow_init();
    test_basic_store_load();
    test_sparse_allocation();
    test_alignment();
    test_overwrite();
    printf("All shadow tests passed!\n");
    return 0;
}
