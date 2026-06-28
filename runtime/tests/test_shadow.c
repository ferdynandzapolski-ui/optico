#include "go_shadow.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {
    printf("Starting GOIR Shadow Store Unit Test...\n");

    // 1. Basic store and load
    void* p1 = (void*)0x123456780;
    go_grade_t g1 = { .base = 0x1000, .end = 0x2000, .perms = 0x3 };
    __go_shadow_store(p1, g1);

    go_grade_t r1 = __go_shadow_load(p1);
    assert(r1.base == g1.base);
    assert(r1.end == g1.end);
    assert(r1.perms == g1.perms);
    printf("  [PASS] Basic store/load\n");

    // 2. Uninitialized load (should return TOP)
    void* p2 = (void*)0x223456780;
    go_grade_t r2 = __go_shadow_load(p2);
    assert(r2.base == 0);
    assert(r2.end == -1ULL);
    assert(r2.perms == 0xF);
    printf("  [PASS] Uninitialized load returns TOP\n");

    // 3. Unaligned access
    void* p3 = (void*)0x123456784; // 4-byte aligned, not 8
    go_grade_t g3 = { .base = 0x3000, .end = 0x4000, .perms = 0x1 };
    __go_shadow_store(p3, g3); // should be ignored
    go_grade_t r3 = __go_shadow_load(p3);
    assert(r3.end == -1ULL); // TOP
    printf("  [PASS] Unaligned access handled correctly\n");

    // 4. Multiple entries in same secondary table
    void* p4 = (void*)0x123456788;
    go_grade_t g4 = { .base = 0x5000, .end = 0x6000, .perms = 0x2 };
    __go_shadow_store(p4, g4);

    r1 = __go_shadow_load(p1);
    go_grade_t r4 = __go_shadow_load(p4);
    assert(r1.base == g1.base);
    assert(r4.base == g4.base);
    printf("  [PASS] Multiple entries in same secondary table\n");

    // 5. Entries in different secondary tables
    void* p5 = (void*)((uintptr_t)p1 + (1UL << 26)); // Cross 64MB boundary to trigger new secondary table
    go_grade_t g5 = { .base = 0x7000, .end = 0x8000, .perms = 0x4 };
    __go_shadow_store(p5, g5);

    go_grade_t r5 = __go_shadow_load(p5);
    assert(r5.base == g5.base);
    r1 = __go_shadow_load(p1);
    assert(r1.base == g1.base);
    printf("  [PASS] Entries in different secondary tables\n");

    // 6. Test 48-byte record size and alignment
    assert(sizeof(go_grade_t) == 48);
    printf("  [PASS] go_grade_t size is 48 bytes\n");

    printf("GOIR Shadow Store Unit Test: ALL PASS\n");
    return 0;
}
