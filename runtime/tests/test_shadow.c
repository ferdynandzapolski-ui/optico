#include "go_shadow.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void test_basic_store_load() {
    printf("Running test_basic_store_load...\n");
    go_grade_t g = {0};
    g.base = 0x1000;
    g.end = 0x2000;
    g.perms = 0x7;

    void *ptr = (void *)0x8000;
    __go_shadow_store(ptr, g);

    go_grade_t g2 = __go_shadow_load(ptr);
    assert(g2.base == g.base);
    assert(g2.end == g.end);
    assert(g2.perms == g.perms);
    printf("  Passed!\n");
}

void test_default_top() {
    printf("Running test_default_top...\n");
    void *ptr = (void *)0x9000;
    go_grade_t g = __go_shadow_load(ptr);
    assert(g.base == 0);
    assert(g.end == -1ULL);
    assert(g.perms == 0xF);
    printf("  Passed!\n");
}

void test_large_address() {
    printf("Running test_large_address...\n");
    go_grade_t g = {0};
    g.base = 0xdeadbeef;

    // Test an address near the top of the 48-bit range
    void *ptr = (void *)((1UL << 47) | 0x1000);
    __go_shadow_store(ptr, g);

    go_grade_t g2 = __go_shadow_load(ptr);
    assert(g2.base == g.base);
    printf("  Passed!\n");
}

int main() {
    __go_shadow_init();
    test_basic_store_load();
    test_default_top();
    test_large_address();
    printf("All tests passed!\n");
    return 0;
}
