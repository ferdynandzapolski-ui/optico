#include "go_shadow.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

void test_basic_store_load() {
    printf("Running test_basic_store_load...\n");
    void* ptr = (void*)0x123456780;
    go_grade_t g = {0};
    g.base = 0x1000;
    g.end = 0x2000;
    g.perms = 0x3;

    __go_shadow_store(ptr, g);
    go_grade_t loaded = __go_shadow_load(ptr);

    assert(loaded.base == g.base);
    assert(loaded.end == g.end);
    assert(loaded.perms == g.perms);
    printf("test_basic_store_load passed.\n");
}

void test_top_grade_default() {
    printf("Running test_top_grade_default...\n");
    void* ptr = (void*)0x888888880;
    go_grade_t loaded = __go_shadow_load(ptr);

    assert(loaded.end == -1ULL);
    assert(loaded.perms == 0xF);
    printf("test_top_grade_default passed.\n");
}

void test_sparse_allocation() {
    printf("Running test_sparse_allocation...\n");
    void* ptr1 = (void*)0x10000000000; // Far away addresses
    void* ptr2 = (void*)0x20000000000;

    go_grade_t g1 = {0x100, 0x200, 1, 1, 1, 0, 0, 0};
    go_grade_t g2 = {0x300, 0x400, 2, 2, 2, 0, 0, 0};

    __go_shadow_store(ptr1, g1);
    __go_shadow_store(ptr2, g2);

    go_grade_t l1 = __go_shadow_load(ptr1);
    go_grade_t l2 = __go_shadow_load(ptr2);

    assert(l1.base == g1.base && l1.alloc_id == g1.alloc_id);
    assert(l2.base == g2.base && l2.alloc_id == g2.alloc_id);
    printf("test_sparse_allocation passed.\n");
}

void test_alignment() {
    printf("Running test_alignment...\n");
    void* ptr = (void*)0x123456781; // Unaligned
    // __go_shadow_store(ptr, g); // This would trigger assert in debug builds

    go_grade_t loaded = __go_shadow_load(ptr);
    assert(loaded.end == -1ULL); // Should return TOP for unaligned
    printf("test_alignment passed.\n");
}

int main() {
    test_basic_store_load();
    test_top_grade_default();
    test_sparse_allocation();
    test_alignment();
    printf("All shadow tests passed!\n");
    return 0;
}
