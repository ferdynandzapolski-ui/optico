#include "go_shadow.h"
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

void test_basic_store_load() {
    printf("Running test_basic_store_load...\n");
    void* ptr = (void*)0x100020003000ULL;
    go_grade_t g = {0};
    g.base = 0x1000;
    g.end = 0x2000;
    g.perms = 0x3;

    __go_shadow_store(ptr, g);
    go_grade_t g2 = __go_shadow_load(ptr);

    assert(g2.base == g.base);
    assert(g2.end == g.end);
    assert(g2.perms == g.perms);
    printf("test_basic_store_load passed!\n");
}

void test_top_grade_fallback() {
    printf("Running test_top_grade_fallback...\n");
    void* ptr = (void*)0x200030004000ULL;
    go_grade_t g = __go_shadow_load(ptr);

    assert(g.base == 0);
    assert(g.end == -1ULL);
    assert(g.perms == 0xF);
    printf("test_top_grade_fallback passed!\n");
}

void test_sparse_allocation() {
    printf("Running test_sparse_allocation...\n");
    // Test addresses in different 32GB regions (different primary indices)
    void* ptr1 = (void*)0x100000000000ULL;
    void* ptr2 = (void*)0x200000000000ULL;
    void* ptr3 = (void*)0x300000000000ULL;

    go_grade_t g1 = {0x1, 0x1, 0, 0, 0x1, 0, 0, 0};
    go_grade_t g2 = {0x2, 0x2, 0, 0, 0x2, 0, 0, 0};
    go_grade_t g3 = {0x3, 0x3, 0, 0, 0x3, 0, 0, 0};

    __go_shadow_store(ptr1, g1);
    __go_shadow_store(ptr2, g2);
    __go_shadow_store(ptr3, g3);

    go_grade_t r1 = __go_shadow_load(ptr1);
    go_grade_t r2 = __go_shadow_load(ptr2);
    go_grade_t r3 = __go_shadow_load(ptr3);

    assert(r1.perms == 0x1);
    assert(r2.perms == 0x2);
    assert(r3.perms == 0x3);
    printf("test_sparse_allocation passed!\n");
}

void test_alignment() {
    printf("Running test_alignment...\n");
    void* unaligned_ptr = (void*)0x100020003001ULL;
    go_grade_t g = __go_shadow_load(unaligned_ptr);
    assert(g.end == -1ULL);
    printf("test_alignment passed!\n");
}

int main() {
    test_basic_store_load();
    test_top_grade_fallback();
    test_sparse_allocation();
    test_alignment();
    printf("All tests passed!\n");
    return 0;
}
