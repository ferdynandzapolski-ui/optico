#include "go_shadow.h"
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <pthread.h>

void test_basic_store_load() {
    printf("Testing basic store/load...\n");
    void* ptr = (void*)0x123456780; // 8-byte aligned
    go_grade_t g = {0};
    g.base = 0x1000;
    g.end = 0x2000;
    g.perms = 0x7;

    __go_shadow_store(ptr, g);
    go_grade_t loaded = __go_shadow_load(ptr);

    assert(loaded.base == g.base);
    assert(loaded.end == g.end);
    assert(loaded.perms == g.perms);
    printf("Basic store/load passed!\n");
}

void test_top_grade_default() {
    printf("Testing TOP grade default...\n");
    void* ptr = (void*)0x888888880;
    go_grade_t loaded = __go_shadow_load(ptr);

    assert(loaded.base == 0);
    assert(loaded.end == -1ULL);
    assert(loaded.perms == 0xF);
    printf("TOP grade default passed!\n");
}

#define THREAD_COUNT 10
#define ITERATIONS 1000

void* thread_func(void* arg) {
    uintptr_t base_ptr = 0x200000000;
    for (int i = 0; i < ITERATIONS; ++i) {
        void* ptr = (void*)(base_ptr + (i * 8));
        go_grade_t g = {0};
        g.base = (uint64_t)ptr;
        g.end = g.base + 8;
        __go_shadow_store(ptr, g);

        go_grade_t loaded = __go_shadow_load(ptr);
        assert(loaded.base == g.base);
    }
    return NULL;
}

void test_concurrency() {
    printf("Testing concurrency...\n");
    pthread_t threads[THREAD_COUNT];
    for (int i = 0; i < THREAD_COUNT; ++i) {
        pthread_create(&threads[i], NULL, thread_func, NULL);
    }
    for (int i = 0; i < THREAD_COUNT; ++i) {
        pthread_join(threads[i], NULL);
    }
    printf("Concurrency test passed!\n");
}

int main() {
    test_basic_store_load();
    test_top_grade_default();
    test_concurrency();
    printf("All shadow tests passed!\n");
    return 0;
}
