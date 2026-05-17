#include "go_shadow.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

void test_basic_store_load() {
    printf("Testing basic store/load...\n");
    // Ensure 8-byte alignment for the slot address
    uint64_t slot __attribute__((aligned(8))) = 0;
    void* ptr = &slot;

    go_grade_t g1;
    memset(&g1, 0, sizeof(g1));
    g1.base = (uint64_t)ptr;
    g1.end = (uint64_t)ptr + 8;
    g1.alloc_id = 100;

    __go_shadow_store(ptr, g1);

    go_grade_t g2 = __go_shadow_load(ptr);

    assert(g2.base == g1.base);
    assert(g2.end == g1.end);
    assert(g2.alloc_id == g1.alloc_id);
    printf("Basic store/load passed.\n");
}

void test_sparse_allocation() {
    printf("Testing sparse allocation...\n");
    // Use an address far away from the stack/heap, must be 8-byte aligned
    void* far_ptr = (void*)0x700000000000ULL;

    go_grade_t g_far;
    memset(&g_far, 0, sizeof(g_far));
    g_far.base = (uint64_t)far_ptr;
    g_far.end = (uint64_t)far_ptr + 8;
    g_far.alloc_id = 200;

    __go_shadow_store(far_ptr, g_far);

    go_grade_t g_load = __go_shadow_load(far_ptr);

    assert(g_load.base == g_far.base);
    assert(g_load.alloc_id == 200);
    printf("Sparse allocation passed.\n");
}

void test_uninitialized_load() {
    printf("Testing uninitialized load...\n");
    uint64_t slot_uninit __attribute__((aligned(8))) = 0;
    void* ptr_uninit = &slot_uninit;

    go_grade_t g = __go_shadow_load(ptr_uninit);

    // Default TOP grade should have end = -1ULL and perms = 0xF
    assert(g.end == -1ULL);
    assert(g.perms == 0xF);
    printf("Uninitialized load passed.\n");
}

int main() {
    test_basic_store_load();
    test_sparse_allocation();
    test_uninitialized_load();
    printf("All shadow store tests passed!\n");
    return 0;
}
