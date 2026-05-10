#include "go_shadow.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

int main() {
    printf("Testing Shadow Metadata Store...\n");

    __go_shadow_init();

    go_grade_t g1 = { .base = 0x1000, .end = 0x2000, .alloc_id = 1, .perms = 3 };
    go_grade_t g2 = { .base = 0x3000, .end = 0x4000, .alloc_id = 2, .perms = 1 };

    void* ptr1 = (void*)0x7ffff7001000;
    void* ptr2 = (void*)0x7ffff7001008;
    void* ptr3 = (void*)0x10002000; // Far away address

    printf("Storing grades...\n");
    __go_shadow_store(ptr1, g1);
    __go_shadow_store(ptr2, g2);
    __go_shadow_store(ptr3, g1);

    printf("Loading grades...\n");
    go_grade_t r1 = __go_shadow_load(ptr1);
    go_grade_t r2 = __go_shadow_load(ptr2);
    go_grade_t r3 = __go_shadow_load(ptr3);
    go_grade_t r4 = __go_shadow_load((void*)0xdeadbeef0); // Never stored

    assert(r1.base == g1.base && r1.end == g1.end && r1.alloc_id == g1.alloc_id);
    assert(r2.base == g2.base && r2.end == g2.end && r2.alloc_id == g2.alloc_id);
    assert(r3.base == g1.base && r3.end == g1.end && r3.alloc_id == g1.alloc_id);

    // TOP grade check
    assert(r4.end == -1ULL);
    assert(r4.perms == 0xF);

    printf("All shadow tests passed!\n");
    return 0;
}
