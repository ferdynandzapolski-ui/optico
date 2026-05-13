#include "go_shadow.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

int main() {
    printf("Testing shadow store...\n");

    go_grade_t g1 = {0};
    g1.base = 0x1000;
    g1.end = 0x2000;
    g1.alloc_id = 42;

    void* ptr1 = (void*)0x800000;
    __go_shadow_store(ptr1, g1);

    go_grade_t g1_back = __go_shadow_load(ptr1);
    assert(g1_back.base == g1.base);
    assert(g1_back.end == g1.end);
    assert(g1_back.alloc_id == g1.alloc_id);

    printf("g1 store/load OK\n");

    go_grade_t g2 = {0};
    g2.base = 0x3000;
    g2.end = 0x4000;
    g2.alloc_id = 99;

    void* ptr2 = (void*)0x123456780;
    __go_shadow_store(ptr2, g2);

    go_grade_t g2_back = __go_shadow_load(ptr2);
    assert(g2_back.base == g2.base);
    assert(g2_back.end == g2.end);
    assert(g2_back.alloc_id == g2.alloc_id);

    printf("g2 store/load OK\n");

    // Test unallocated lookup
    void* ptr3 = (void*)0xFFFFFFFF0;
    go_grade_t g3_back = __go_shadow_load(ptr3);
    assert(g3_back.end == -1ULL); // TOP grade
    assert(g3_back.perms == 0xF);

    printf("unallocated lookup OK\n");

    // Test overlapping (multiple of 8)
    void* ptr4 = (void*)0x800008;
    go_grade_t g4 = {0};
    g4.base = 0x5000;
    __go_shadow_store(ptr4, g4);

    g1_back = __go_shadow_load(ptr1);
    assert(g1_back.base == g1.base); // Should not be affected

    go_grade_t g4_back = __go_shadow_load(ptr4);
    assert(g4_back.base == g4.base);

    printf("overlapping multiple of 8 OK\n");

    printf("All shadow store tests passed!\n");
    return 0;
}
