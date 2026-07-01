#include "go_shadow.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void test_basic_shadow() {
    printf("Testing basic shadow store/load...\n");
    int x;
    void* ptr = &x;
    // Align to 8 bytes for the test
    void* aligned_ptr = (void*)(((uintptr_t)ptr) & ~0x7ULL);

    go_grade_t g1 = {100, 200, 1, 1, 0x3, 1, 0, 0};
    __go_shadow_store(aligned_ptr, g1);

    go_grade_t g2 = __go_shadow_load(aligned_ptr);

    assert(g1.base == g2.base);
    assert(g1.end == g2.end);
    assert(g1.alloc_id == g2.alloc_id);
    assert(g1.perms == g2.perms);
    printf("Basic shadow store/load passed.\n");
}

void test_unaligned_shadow() {
    printf("Testing unaligned shadow access...\n");
    char buffer[16];
    void* unaligned_ptr = &buffer[1];
    if (((uintptr_t)unaligned_ptr) & 0x7) {
        go_grade_t g = {100, 200, 1, 1, 0x3, 1, 0, 0};
        __go_shadow_store(unaligned_ptr, g); // Should be ignored

        go_grade_t loaded = __go_shadow_load(unaligned_ptr);
        // Should return TOP
        assert(loaded.base == 0);
        assert(loaded.end == -1ULL);
        assert(loaded.perms == 0xF);
    }
    printf("Unaligned shadow access passed.\n");
}

void test_multi_level_allocation() {
    printf("Testing multi-level table allocation...\n");
    // Use addresses in different primary/secondary regions
    void* p1 = (void*)0x100000000ULL; // Primary 0, Secondary something
    void* p2 = (void*)0x200000000ULL; // Primary something else

    go_grade_t g1 = {1, 2, 3, 4, 5, 6, 7, 8};
    go_grade_t g2 = {8, 7, 6, 5, 4, 3, 2, 1};

    __go_shadow_store(p1, g1);
    __go_shadow_store(p2, g2);

    go_grade_t l1 = __go_shadow_load(p1);
    go_grade_t l2 = __go_shadow_load(p2);

    assert(memcmp(&g1, &l1, sizeof(go_grade_t)) == 0);
    assert(memcmp(&g2, &l2, sizeof(go_grade_t)) == 0);
    printf("Multi-level table allocation passed.\n");
}

int main() {
    test_basic_shadow();
    test_unaligned_shadow();
    test_multi_level_allocation();
    printf("All shadow tests passed!\n");
    return 0;
}
