#include "go_shadow.h"
#include <assert.h>
#include <stdio.h>

int main() {
    __go_shadow_init();

    go_grade_t g1 = {0};
    g1.base = 0x1000;
    g1.end = 0x2000;
    g1.perms = 0x3;

    void* ptr1 = (void*)0x7f0000001000;
    __go_shadow_store(ptr1, g1);

    go_grade_t g2 = __go_shadow_load(ptr1);
    assert(g2.base == g1.base);
    assert(g2.end == g1.end);
    assert(g2.perms == g1.perms);

    // Test O(1) lookup in same secondary table
    void* ptr2 = (void*)0x7f0000001008;
    go_grade_t g3 = {0};
    g3.base = 0x3000;
    g3.end = 0x4000;
    __go_shadow_store(ptr2, g3);

    go_grade_t g4 = __go_shadow_load(ptr2);
    assert(g4.base == g3.base);

    // Test different secondary table
    void* ptr3 = (void*)0x7e0000001000;
    go_grade_t g5 = {0};
    g5.base = 0x5000;
    __go_shadow_store(ptr3, g5);
    assert(__go_shadow_load(ptr3).base == 0x5000);

    // Test uninitialized load
    void* ptr4 = (void*)0x7d0000001000;
    go_grade_t g6 = __go_shadow_load(ptr4);
    assert(g6.end == -1ULL);
    assert(g6.perms == 0xF);

    printf("Shadow store unit tests passed!\n");
    return 0;
}
