#include "go_shadow.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void test_basic_shadow() {
    printf("Running test_basic_shadow...\n");
    void* ptr1 = (void*)0x12345678000;
    go_grade_t g1 = {0};
    g1.base = 0x12345000;
    g1.end = 0x12346000;
    g1.alloc_id = 1;

    __go_shadow_store(ptr1, g1);
    go_grade_t g1_out = __go_shadow_load(ptr1);

    assert(g1_out.base == g1.base);
    assert(g1_out.end == g1.end);
    assert(g1_out.alloc_id == g1.alloc_id);
    printf("test_basic_shadow passed.\n");
}

void test_multiple_slots() {
    printf("Running test_multiple_slots...\n");
    void* ptr1 = (void*)0x12345678000;
    void* ptr2 = (void*)0x12345678008;
    void* ptr3 = (void*)0x12345678010;

    go_grade_t g1 = {0}; g1.alloc_id = 1;
    go_grade_t g2 = {0}; g2.alloc_id = 2;
    go_grade_t g3 = {0}; g3.alloc_id = 3;

    __go_shadow_store(ptr1, g1);
    __go_shadow_store(ptr2, g2);
    __go_shadow_store(ptr3, g3);

    assert(__go_shadow_load(ptr1).alloc_id == 1);
    assert(__go_shadow_load(ptr2).alloc_id == 2);
    assert(__go_shadow_load(ptr3).alloc_id == 3);
    printf("test_multiple_slots passed.\n");
}

void test_different_regions() {
    printf("Running test_different_regions...\n");
    void* ptr1 = (void*)0x10000000000;
    void* ptr2 = (void*)0x70000000000;

    go_grade_t g1 = {0}; g1.alloc_id = 10;
    go_grade_t g2 = {0}; g2.alloc_id = 20;

    __go_shadow_store(ptr1, g1);
    __go_shadow_store(ptr2, g2);

    assert(__go_shadow_load(ptr1).alloc_id == 10);
    assert(__go_shadow_load(ptr2).alloc_id == 20);
    printf("test_different_regions passed.\n");
}

int main() {
    test_basic_shadow();
    test_multiple_slots();
    test_different_regions();
    printf("All shadow tests passed!\n");
    return 0;
}
