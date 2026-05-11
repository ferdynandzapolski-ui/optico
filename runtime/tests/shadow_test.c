#include "go_shadow.h"
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

int main() {
    printf("Starting shadow table tests...\n");

    // Initialize (should be automatic but we can call it)
    __go_shadow_init();

    go_grade_t g1 = {100, 200, 1, 1, 0xF, 0, 0, 1};
    void* slot1 = (void*)0x123456780; // 8-byte aligned

    printf("Storing grade g1 at %p\n", slot1);
    __go_shadow_store(slot1, g1);

    printf("Loading grade from %p\n", slot1);
    go_grade_t r1 = __go_shadow_load(slot1);

    assert(r1.base == g1.base);
    assert(r1.end == g1.end);
    assert(r1.alloc_id == g1.alloc_id);
    printf("Test 1 passed (Basic store/load)\n");

    // Test default TOP grade
    void* slot2 = (void*)0x987654320;
    printf("Loading uninitialized grade from %p\n", slot2);
    go_grade_t r2 = __go_shadow_load(slot2);
    assert(r2.base == 0);
    assert(r2.end == -1ULL);
    assert(r2.perms == 0xF);
    printf("Test 2 passed (Default TOP grade)\n");

    // Test another slot in same primary but different secondary
    void* slot3 = (void*)0x123456788;
    go_grade_t g3 = {300, 400, 2, 2, 0xF, 0, 0, 1};
    printf("Storing grade g3 at %p\n", slot3);
    __go_shadow_store(slot3, g3);
    go_grade_t r3 = __go_shadow_load(slot3);
    assert(r3.base == g3.base);
    printf("Test 3 passed (Adjacent slots)\n");

    // Test a very different address
    void* slot4 = (void*)0x7FFF12345678; // High address
    go_grade_t g4 = {500, 600, 3, 3, 0xF, 0, 0, 1};
    void* aligned_slot4 = (void*)((uintptr_t)slot4 & ~0x7ULL);
    printf("Storing grade g4 at %p\n", aligned_slot4);
    __go_shadow_store(aligned_slot4, g4);
    go_grade_t r4 = __go_shadow_load(aligned_slot4);
    assert(r4.base == g4.base);
    printf("Test 4 passed (High address)\n");

    printf("All shadow table tests passed!\n");
    return 0;
}
