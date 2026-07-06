#include "go_shadow.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void test_basic_store_load() {
    printf("Testing basic store/load...\n");
    int x = 42;
    void* ptr = &x;

    // Ensure 8-byte alignment for the test
    uintptr_t addr = ((uintptr_t)ptr) & ~0x7ULL;
    ptr = (void*)addr;

    go_grade_t g = {0};
    g.base = 0x1000;
    g.end = 0x2000;
    g.perms = 0x3;

    __go_shadow_store(ptr, g);
    go_grade_t g_loaded = __go_shadow_load(ptr);

    assert(g_loaded.base == g.base);
    assert(g_loaded.end == g.end);
    assert(g_loaded.perms == g.perms);
    printf("PASS\n");
}

void test_unaligned_access() {
    printf("Testing unaligned access...\n");
    char buffer[16];
    void* ptr = &buffer[1]; // Unaligned

    go_grade_t g = {0};
    g.base = 0x1000;
    g.end = 0x2000;
    g.perms = 0x3;

    __go_shadow_store(ptr, g);
    go_grade_t g_loaded = __go_shadow_load(ptr);

    // Should return TOP for unaligned load
    assert(g_loaded.base == 0);
    assert(g_loaded.end == -1ULL);
    assert(g_loaded.perms == 0xF);
    printf("PASS\n");
}

void test_sparse_allocation() {
    printf("Testing sparse allocation...\n");
    // Pick two addresses in different primary slots
    void* ptr1 = (void*)0x1000000000ULL; // High address
    void* ptr2 = (void*)0x2000000000ULL; // Another high address

    go_grade_t g1 = {0}; g1.base = 0x100;
    go_grade_t g2 = {0}; g2.base = 0x200;

    __go_shadow_store(ptr1, g1);
    __go_shadow_store(ptr2, g2);

    go_grade_t r1 = __go_shadow_load(ptr1);
    go_grade_t r2 = __go_shadow_load(ptr2);

    assert(r1.base == 0x100);
    assert(r2.base == 0x200);
    printf("PASS\n");
}

int main() {
    test_basic_store_load();
    test_unaligned_access();
    test_sparse_allocation();
    printf("All shadow tests passed!\n");
    return 0;
}
