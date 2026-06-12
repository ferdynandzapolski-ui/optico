#include "go_shadow.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

void test_basic_store_load() {
    printf("Testing basic store/load...\n");
    void* ptr = (void*)0x123456780000ULL;
    go_grade_t g = {0};
    g.base = 0x1000;
    g.end = 0x2000;
    g.perms = 0x7;

    __go_shadow_store(ptr, g);
    go_grade_t g2 = __go_shadow_load(ptr);

    assert(g2.base == g.base);
    assert(g2.end == g.end);
    assert(g2.perms == g.perms);
    printf("Basic store/load PASSED.\n");
}

void test_uninitialized_load() {
    printf("Testing uninitialized load...\n");
    void* ptr = (void*)0x888888880000ULL;
    go_grade_t g = __go_shadow_load(ptr);

    // Should return TOP
    assert(g.end == -1ULL);
    assert(g.perms == 0xF);
    printf("Uninitialized load PASSED.\n");
}

void test_sparsity() {
    printf("Testing sparsity...\n");
    void* ptr1 = (void*)0x100000000000ULL;
    void* ptr2 = (void*)0x700000000000ULL; // High address

    go_grade_t g1 = {0x1, 0x2, 0, 0, 1, 0, 0, 0};
    go_grade_t g2 = {0x3, 0x4, 0, 0, 2, 0, 0, 0};

    __go_shadow_store(ptr1, g1);
    __go_shadow_store(ptr2, g2);

    go_grade_t r1 = __go_shadow_load(ptr1);
    go_grade_t r2 = __go_shadow_load(ptr2);

    assert(r1.base == 0x1);
    assert(r2.base == 0x3);
    printf("Sparsity PASSED.\n");
}

void test_sequential() {
    printf("Testing sequential stores to same slot...\n");
    void* ptr = (void*)0x999999990000ULL;
    go_grade_t g1 = {0x1, 0x10, 0, 0, 1, 0, 0, 0};
    go_grade_t g2 = {0x2, 0x20, 0, 0, 2, 0, 0, 0};

    __go_shadow_store(ptr, g1);
    assert(__go_shadow_load(ptr).base == 0x1);

    __go_shadow_store(ptr, g2);
    assert(__go_shadow_load(ptr).base == 0x2);
    printf("Sequential PASSED.\n");
}

int main() {
    __go_shadow_init();
    test_basic_store_load();
    test_uninitialized_load();
    test_sparsity();
    test_sequential();
    printf("All shadow store unit tests PASSED.\n");
    return 0;
}
