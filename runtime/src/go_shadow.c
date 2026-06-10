#include "go_shadow.h"
#include "go_metrics.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>
#include <assert.h>

#define PRIMARY_BITS 22
#define SECONDARY_BITS 23
#define PRIMARY_SIZE (1ULL << PRIMARY_BITS)
#define SECONDARY_SIZE (1ULL << SECONDARY_BITS)

#define PRIMARY_INDEX(addr) (((uintptr_t)(addr) >> 26) & (PRIMARY_SIZE - 1))
#define SECONDARY_INDEX(addr) (((uintptr_t)(addr) >> 3) & (SECONDARY_SIZE - 1))

static go_grade_t** primary_table = NULL;

void __go_shadow_init(void) {
    if (atomic_load_explicit((_Atomic(go_grade_t**)*)&primary_table, memory_order_acquire)) return;

    go_grade_t** new_primary = (go_grade_t**)mmap(NULL, PRIMARY_SIZE * sizeof(void*),
                                                  PROT_READ | PROT_WRITE,
                                                  MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (new_primary == MAP_FAILED) {
        perror("GOIR: Failed to mmap primary shadow table");
        abort();
    }

    go_grade_t** expected = NULL;
    if (!atomic_compare_exchange_strong_explicit((_Atomic(go_grade_t**)*)&primary_table,
                                                 &expected, new_primary,
                                                 memory_order_release, memory_order_acquire)) {
        munmap(new_primary, PRIMARY_SIZE * sizeof(void*));
    }
}

static go_grade_t* get_secondary_table(uintptr_t primary_idx) {
    go_grade_t* secondary = atomic_load_explicit((_Atomic(go_grade_t*)*)&primary_table[primary_idx], memory_order_acquire);
    if (secondary) return secondary;

    go_grade_t* new_secondary = (go_grade_t*)mmap(NULL, SECONDARY_SIZE * sizeof(go_grade_t),
                                                   PROT_READ | PROT_WRITE,
                                                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (new_secondary == MAP_FAILED) {
        perror("GOIR: Failed to mmap secondary shadow table");
        abort();
    }

    go_grade_t* expected = NULL;
    if (!atomic_compare_exchange_strong_explicit((_Atomic(go_grade_t*)*)&primary_table[primary_idx],
                                                 &expected, new_secondary,
                                                 memory_order_release, memory_order_acquire)) {
        munmap(new_secondary, SECONDARY_SIZE * sizeof(go_grade_t));
        return expected;
    }
    return new_secondary;
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    __go_metrics_inc(GO_METRIC_SHADOW_STORE);
    if (!primary_table) __go_shadow_init();
    assert(((uintptr_t)slot_addr & 0x7) == 0 && "Shadow slot address must be 8-byte aligned");

    uintptr_t p_idx = PRIMARY_INDEX(slot_addr);
    uintptr_t s_idx = SECONDARY_INDEX(slot_addr);

    go_grade_t* secondary = get_secondary_table(p_idx);
    secondary[s_idx] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    __go_metrics_inc(GO_METRIC_SHADOW_LOAD);
    if (!primary_table) __go_shadow_init();
    if (((uintptr_t)slot_addr & 0x7) != 0) {
        // Unaligned load? Fallback to TOP
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }

    uintptr_t p_idx = PRIMARY_INDEX(slot_addr);
    uintptr_t s_idx = SECONDARY_INDEX(slot_addr);

    go_grade_t* secondary = atomic_load_explicit((_Atomic(go_grade_t*)*)&primary_table[p_idx], memory_order_acquire);
    if (!secondary) {
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }

    go_grade_t g = secondary[s_idx];
    if (g.end == 0) { // Uninitialized entry
        g.end = -1ULL;
        g.perms = 0xF;
    }
    return g;
}
