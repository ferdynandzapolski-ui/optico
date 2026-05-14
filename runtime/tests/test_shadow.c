#include "goirrt.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

int main() {
    printf("Running shadow store tests...\n");

    uint64_t dummy_data[10];
    void *ptr1 = &dummy_data[0];
    void *ptr2 = &dummy_data[5];

    go_grade_t g1 = {0};
    g1.base = 0x1000;
    g1.end = 0x2000;
    g1.perms = 0x1;

    go_grade_t g2 = {0};
    g2.base = 0x3000;
    g2.end = 0x4000;
    g2.perms = 0x2;

    // Test store and load
    __go_shadow_store(ptr1, g1);
    __go_shadow_store(ptr2, g2);

    go_grade_t r1 = __go_shadow_load(ptr1);
    go_grade_t r2 = __go_shadow_load(ptr2);

    assert(r1.base == g1.base);
    assert(r1.end == g1.end);
    assert(r1.perms == g1.perms);

    assert(r2.base == g2.base);
    assert(r2.end == g2.end);
    assert(r2.perms == g2.perms);

    // Test default TOP grade for unallocated slots
    void *ptr3 = &dummy_data[2];
    go_grade_t r3 = __go_shadow_load(ptr3);
    assert(r3.base == 0);
    assert(r3.end == -1ULL);
    assert(r3.perms == 0xF);

    // Test overwrite
    go_grade_t g1_new = g1;
    g1_new.perms = 0x3;
    __go_shadow_store(ptr1, g1_new);
    r1 = __go_shadow_load(ptr1);
    assert(r1.perms == 0x3);

    printf("Shadow store tests passed!\n");
    return 0;
}
