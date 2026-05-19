#include "goirrt.h"
#include "go_shadow.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

int main() {
    printf("Testing Shadow Store...\n");

    go_grade_t g1 = {100, 200, 1, 0, 0xF, 0, 0, 0};
    uintptr_t slot1 = 0x10008; // 8-byte aligned

    __go_shadow_store((void*)slot1, g1);
    printf("Stored grade g1 at %p\n", (void*)slot1);

    go_grade_t r1 = __go_shadow_load((void*)slot1);
    printf("Loaded grade: base=%lu, end=%lu\n", r1.base, r1.end);
    assert(r1.base == 100);
    assert(r1.end == 200);
    assert(r1.alloc_id == 1);

    // Test default TOP grade for uninitialized slot
    uintptr_t slot2 = 0x20008;
    go_grade_t r2 = __go_shadow_load((void*)slot2);
    printf("Loaded uninitialized grade: end=%lu, perms=0x%x\n", r2.end, r2.perms);
    assert(r2.end == -1ULL);
    assert(r2.perms == 0xF);

    // Test across secondary table boundaries
    uintptr_t slot3 = 0x100000008ULL; // Should be in a different secondary table
    go_grade_t g3 = {500, 600, 3, 0, 0xF, 0, 0, 0};
    __go_shadow_store((void*)slot3, g3);
    go_grade_t r3 = __go_shadow_load((void*)slot3);
    assert(r3.base == 500);
    assert(r3.alloc_id == 3);

    printf("Shadow Store Tests Passed!\n");
    return 0;
}
