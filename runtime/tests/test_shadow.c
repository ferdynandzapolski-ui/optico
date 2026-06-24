#include "goirrt.h"
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

void test_basic_store_load() {
    printf("Testing basic store/load...\n");
    uint64_t dummy_ptr;
    go_grade_t g = {0};
    g.base = 0x1000;
    g.end = 0x2000;
    g.perms = 0x3;

    __go_shadow_store(&dummy_ptr, g);
    go_grade_t g2 = __go_shadow_load(&dummy_ptr);

    assert(g2.base == g.base);
    assert(g2.end == g.end);
    assert(g2.perms == g.perms);
    printf("PASS\n");
}

void test_uninitialized_load() {
    printf("Testing uninitialized load (should return TOP)...\n");
    uint64_t dummy_ptr2;
    go_grade_t g = __go_shadow_load(&dummy_ptr2);

    assert(g.base == 0);
    assert(g.end == -1ULL);
    assert(g.perms == 0xF);
    printf("PASS\n");
}

void test_unaligned_load() {
    printf("Testing unaligned load (should return TOP)...\n");
    char buffer[16];
    void* unaligned = &buffer[1];
    go_grade_t g = __go_shadow_load(unaligned);

    assert(g.end == -1ULL);
    assert(g.perms == 0xF);
    printf("PASS\n");
}

void test_lazy_allocation() {
    printf("Testing lazy allocation across primary boundaries...\n");
    // Force allocation in different secondary tables
    void* addr1 = (void*)0x100000000ULL; // 4GB
    void* addr2 = (void*)0x200000000ULL; // 8GB

    go_grade_t g1 = {0x100, 0x200, 0, 0, 0x1, 0, 0, 0, 0};
    go_grade_t g2 = {0x300, 0x400, 0, 0, 0x2, 0, 0, 0, 0};

    __go_shadow_store(addr1, g1);
    __go_shadow_store(addr2, g2);

    go_grade_t r1 = __go_shadow_load(addr1);
    go_grade_t r2 = __go_shadow_load(addr2);

    assert(r1.base == g1.base && r1.perms == g1.perms);
    assert(r2.base == g2.base && r2.perms == g2.perms);
    printf("PASS\n");
}

int main() {
    __go_shadow_init();
    test_basic_store_load();
    test_uninitialized_load();
    test_unaligned_load();
    test_lazy_allocation();
    printf("All shadow tests passed!\n");
    return 0;
}
