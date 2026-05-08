#include "go_shadow.h"
#include <assert.h>
#include <stdio.h>

int main() {
    __go_shadow_init();

    go_grade_t g1 = {0};
    g1.base = 0x1000;
    g1.end = 0x2000;
    g1.alloc_id = 42;

    void* ptr1 = (void*)0x8000;
    __go_shadow_store(ptr1, g1);

    go_grade_t r1 = __go_shadow_load(ptr1);
    assert(r1.base == g1.base);
    assert(r1.end == g1.end);
    assert(r1.alloc_id == g1.alloc_id);

    // Test a different address in a different primary slot
    void* ptr2 = (void*)(0x8000 + (1UL << 26));
    go_grade_t g2 = {0};
    g2.base = 0x3000;
    g2.end = 0x4000;
    __go_shadow_store(ptr2, g2);

    go_grade_t r2 = __go_shadow_load(ptr2);
    assert(r2.base == g2.base);
    assert(r1.base == g1.base); // Ensure no interference

    // Test TOP grade for unmapped address
    void* ptr3 = (void*)0x9000;
    go_grade_t r3 = __go_shadow_load(ptr3);
    assert(r3.end == -1ULL);
    assert(r3.perms == 0xF);

    printf("Shadow store unit tests passed!\n");
    return 0;
}
