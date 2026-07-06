#include "go_shadow.h"
#include <sys/mman.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#ifndef MAP_NORESERVE
#define MAP_NORESERVE 0
#endif

// 48-bit address space parameters
// Bits 47-26 (22 bits) -> Primary
// Bits 25-3 (23 bits) -> Secondary (8-byte aligned slots)
#define GO_PRIMARY_BITS 22
#define GO_SECONDARY_BITS 23
#define GO_SHIFT_BITS 3

#define GO_PRIMARY_ENTRIES (1ULL << GO_PRIMARY_BITS)
#define GO_SECONDARY_ENTRIES (1ULL << GO_SECONDARY_BITS)

typedef _Atomic(go_grade_t*) atomic_secondary_ptr_t;

static _Atomic(atomic_secondary_ptr_t*) primary_table = NULL;

void __go_shadow_init(void) {
    if (atomic_load_explicit(&primary_table, memory_order_acquire) != NULL) return;

    size_t size = sizeof(atomic_secondary_ptr_t) * GO_PRIMARY_ENTRIES;
    void* p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    if (p == MAP_FAILED) {
        perror("GOIR Shadow: Failed to mmap primary table");
        abort();
    }

    atomic_secondary_ptr_t* expected = NULL;
    if (!atomic_compare_exchange_strong_explicit(&primary_table, &expected, (atomic_secondary_ptr_t*)p,
                                                 memory_order_acq_rel, memory_order_acquire)) {
        munmap(p, size);
    }
}

static go_grade_t* get_secondary_table(uintptr_t primary_idx) {
    atomic_secondary_ptr_t* pt = atomic_load_explicit(&primary_table, memory_order_acquire);
    if (!pt) {
        __go_shadow_init();
        pt = atomic_load_explicit(&primary_table, memory_order_acquire);
    }

    go_grade_t* st = atomic_load_explicit(&pt[primary_idx], memory_order_acquire);
    if (st) return st;

    size_t size = sizeof(go_grade_t) * GO_SECONDARY_ENTRIES;
    void* p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    if (p == MAP_FAILED) {
        perror("GOIR Shadow: Failed to mmap secondary table");
        return NULL;
    }

    go_grade_t* expected = NULL;
    if (!atomic_compare_exchange_strong_explicit(&pt[primary_idx], &expected, (go_grade_t*)p,
                                                 memory_order_acq_rel, memory_order_acquire)) {
        munmap(p, size);
        return expected;
    }

    return (go_grade_t*)p;
}

go_grade_t* __go_shadow_map(void* addr) {
    uintptr_t uaddr = (uintptr_t)addr;

    // Check 8-byte alignment
    if (uaddr & ( (1 << GO_SHIFT_BITS) - 1)) return NULL;

    // Check 48-bit address range
    if (uaddr >= (1ULL << 48)) return NULL;

    uintptr_t p_idx = (uaddr >> (GO_SECONDARY_BITS + GO_SHIFT_BITS)) & (GO_PRIMARY_ENTRIES - 1);
    uintptr_t s_idx = (uaddr >> GO_SHIFT_BITS) & (GO_SECONDARY_ENTRIES - 1);

    go_grade_t* st = get_secondary_table(p_idx);
    if (!st) return NULL;

    return &st[s_idx];
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    go_grade_t* slot = __go_shadow_map(slot_addr);
    if (slot) {
        *slot = g;
    }
}

go_grade_t __go_shadow_load(void* slot_addr) {
    go_grade_t* slot = __go_shadow_map(slot_addr);
    if (slot) {
        // If the entry is zeroed (common in sparse mmap), return a default TOP
        if (slot->base == 0 && slot->end == 0 && slot->perms == 0) {
            goto return_top;
        }
        return *slot;
    }

return_top:;
    go_grade_t g_top = {0};
    g_top.end = -1ULL;
    g_top.perms = 0xF;
    return g_top;
}
