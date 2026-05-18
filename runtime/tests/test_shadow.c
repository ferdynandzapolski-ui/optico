#include "go_shadow.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

void test_basic_store_load() {
    printf("Testing basic store/load...\n");
    uint64_t slot;
    go_grade_t g = {0};
    g.base = 0x1000;
    g.end = 0x2000;
    g.perms = 0x3;

    __go_shadow_store(&slot, g);
    go_grade_t g2 = __go_shadow_load(&slot);

    assert(g2.base == g.base);
    assert(g2.end == g.end);
    assert(g2.perms == g.perms);
    printf("  Passed!\n");
}

void test_default_top() {
    printf("Testing default TOP grade...\n");
    uint64_t slot; // Not stored to shadow
    go_grade_t g = __go_shadow_load(&slot);

    assert(g.end == -1ULL);
    assert(g.perms == 0xF);
    printf("  Passed!\n");
}

void test_large_address_space() {
    printf("Testing large address space mapping...\n");
    // Simulate addresses in different primary/secondary regions
    void* addr1 = (void*)0x100000000000ULL; // ~16TB
    void* addr2 = (void*)0x700000000000ULL; // ~112TB

    go_grade_t g1 = {0x100, 0x200, 1, 1, 1, 1, 1, 1};
    go_grade_t g2 = {0x300, 0x400, 2, 2, 2, 2, 2, 2};

    __go_shadow_store(addr1, g1);
    __go_shadow_store(addr2, g2);

    go_grade_t r1 = __go_shadow_load(addr1);
    go_grade_t r2 = __go_shadow_load(addr2);

    assert(r1.base == 0x100);
    assert(r2.base == 0x300);
    printf("  Passed!\n");
}

int main() {
    test_basic_store_load();
    test_default_top();
    test_large_address_space();
    printf("All shadow tests passed!\n");
    return 0;
}
