#include "go_shadow.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void test_basic_store_load() {
    printf("Testing basic store/load...\n");
    int x;
    void* ptr_slot = &x; // Note: might not be 8-byte aligned on some platforms, but usually is for int on 64-bit

    // Ensure 8-byte alignment for the test
    uintptr_t addr = (uintptr_t)ptr_slot;
    addr &= ~7UL;
    ptr_slot = (void*)addr;

    go_grade_t g1 = {0};
    g1.base = 0x1000;
    g1.end = 0x2000;
    g1.perms = 0x3;

    __go_shadow_store(ptr_slot, g1);
    go_grade_t g2 = __go_shadow_load(ptr_slot);

    assert(g2.base == g1.base);
    assert(g2.end == g1.end);
    assert(g2.perms == g1.perms);
    printf("PASS\n");
}

void test_unaligned_access() {
    printf("Testing unaligned access...\n");
    char buffer[16];
    void* unaligned_slot = &buffer[1];
    if (((uintptr_t)unaligned_slot & 7) == 0) unaligned_slot = &buffer[0]; // should not happen with &buffer[1]

    go_grade_t g1 = {0};
    g1.base = 0x3000;

    __go_shadow_store(unaligned_slot, g1);
    go_grade_t g2 = __go_shadow_load(unaligned_slot);

    // Should return TOP grade (end = -1)
    assert(g2.end == -1ULL);
    assert(g2.perms == 0xF);
    printf("PASS\n");
}

void test_uninitialized_load() {
    printf("Testing uninitialized load...\n");
    void* fresh_slot = (void*)0x7FFF00008000ULL; // Far away address

    go_grade_t g = __go_shadow_load(fresh_slot);
    assert(g.end == -1ULL);
    assert(g.perms == 0xF);
    printf("PASS\n");
}

void test_large_address_space() {
    printf("Testing large address space...\n");
    // Test addresses at different ends of a 48-bit address space
    void* low_addr = (void*)0x10008ULL;
    void* high_addr = (void*)0x7FFFFFFFF000ULL;

    go_grade_t g_low = {0}; g_low.base = 0xAAAA;
    go_grade_t g_high = {0}; g_high.base = 0xBBBB;

    __go_shadow_store(low_addr, g_low);
    __go_shadow_store(high_addr, g_high);

    go_grade_t r_low = __go_shadow_load(low_addr);
    go_grade_t r_high = __go_shadow_load(high_addr);

    assert(r_low.base == 0xAAAA);
    assert(r_high.base == 0xBBBB);
    printf("PASS\n");
}

int main() {
    __go_shadow_init();
    test_basic_store_load();
    test_unaligned_access();
    test_uninitialized_load();
    test_large_address_space();
    printf("All shadow tests PASSED\n");
    return 0;
}
