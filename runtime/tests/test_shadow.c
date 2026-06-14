#include "go_shadow.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

void test_basic_store_load() {
    printf("Testing basic store and load...\n");
    void* ptr = (void*)0x123456780; // 8-byte aligned
    go_grade_t g = {0};
    g.base = 0x100;
    g.end = 0x200;
    g.perms = 0x3;

    __go_shadow_store(ptr, g);
    go_grade_t loaded = __go_shadow_load(ptr);

    assert(loaded.base == g.base);
    assert(loaded.end == g.end);
    assert(loaded.perms == g.perms);
    printf("Basic store and load passed.\n");
}

void test_unaligned_access() {
    printf("Testing unaligned access...\n");
    void* ptr = (void*)0x123456781; // Not 8-byte aligned
    go_grade_t g = {0};
    g.base = 0x100;
    g.end = 0x200;

    __go_shadow_store(ptr, g);
    go_grade_t loaded = __go_shadow_load(ptr);

    // Should return TOP grade (end = -1ULL)
    assert(loaded.end == -1ULL);
    printf("Unaligned access passed.\n");
}

void test_uninitialized_access() {
    printf("Testing uninitialized access...\n");
    void* ptr = (void*)0x888888880;
    go_grade_t loaded = __go_shadow_load(ptr);

    assert(loaded.end == -1ULL);
    printf("Uninitialized access passed.\n");
}

void test_sparse_allocation() {
    printf("Testing sparse allocation...\n");
    void* ptr1 = (void*)0x1000000000;
    void* ptr2 = (void*)0x2000000000;

    go_grade_t g1 = { .base = 1, .end = 10 };
    go_grade_t g2 = { .base = 2, .end = 20 };

    __go_shadow_store(ptr1, g1);
    __go_shadow_store(ptr2, g2);

    go_grade_t l1 = __go_shadow_load(ptr1);
    go_grade_t l2 = __go_shadow_load(ptr2);

    assert(l1.base == 1 && l1.end == 10);
    assert(l2.base == 2 && l2.end == 20);
    printf("Sparse allocation passed.\n");
}

int main() {
    test_basic_store_load();
    test_unaligned_access();
    test_uninitialized_access();
    test_sparse_allocation();
    printf("All shadow store unit tests passed!\n");
    return 0;
}
