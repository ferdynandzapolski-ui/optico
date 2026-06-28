#include "go_shadow.h"
#include <sys/mman.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

/*
 * Scalable Two-Level Shadow Metadata Store
 * 48-bit address space, 8-byte aligned slots.
 * Bits 47-26 (22 bits): Primary Table Index
 * Bits 25-3  (23 bits): Secondary Table Index
 */

#ifndef GO_PRIMARY_BITS
#define GO_PRIMARY_BITS 22
#endif

#ifndef GO_SECONDARY_BITS
#define GO_SECONDARY_BITS 23
#endif

#ifndef GO_SHIFT_BITS
#define GO_SHIFT_BITS 3
#endif

#define GO_PRIMARY_ENTRIES (1UL << GO_PRIMARY_BITS)
#define GO_SECONDARY_ENTRIES (1UL << GO_SECONDARY_BITS)

typedef _Atomic(go_grade_t*) atomic_secondary_ptr_t;

static _Atomic(atomic_secondary_ptr_t*) primary_table = NULL;

static void __go_shadow_init() {
    if (atomic_load_explicit(&primary_table, memory_order_acquire) != NULL) {
        return;
    }

    size_t size = GO_PRIMARY_ENTRIES * sizeof(atomic_secondary_ptr_t);
    void* p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (p == MAP_FAILED) {
        perror("GOIR Shadow: mmap primary table failed");
        abort();
    }

    atomic_secondary_ptr_t* expected = NULL;
    if (!atomic_compare_exchange_strong_explicit(&primary_table, &expected, (atomic_secondary_ptr_t*)p,
                                                 memory_order_acq_rel, memory_order_acquire)) {
        munmap(p, size);
    }
}

static go_grade_t* get_secondary_table(size_t primary_idx) {
    atomic_secondary_ptr_t* pt = atomic_load_explicit(&primary_table, memory_order_acquire);
    if (!pt) {
        __go_shadow_init();
        pt = atomic_load_explicit(&primary_table, memory_order_acquire);
    }

    go_grade_t* st = atomic_load_explicit(&pt[primary_idx], memory_order_acquire);
    if (st) return st;

    size_t size = GO_SECONDARY_ENTRIES * sizeof(go_grade_t);
    void* p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (p == MAP_FAILED) {
        perror("GOIR Shadow: mmap secondary table failed");
        abort();
    }

    go_grade_t* expected = NULL;
    if (atomic_compare_exchange_strong_explicit(&pt[primary_idx], &expected, (go_grade_t*)p,
                                                 memory_order_acq_rel, memory_order_acquire)) {
        return (go_grade_t*)p;
    } else {
        munmap(p, size);
        return expected;
    }
}

static go_grade_t get_top() {
    go_grade_t g = {0};
    g.end = -1ULL;
    g.perms = 0xF;
    return g;
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    uintptr_t addr = (uintptr_t)slot_addr;
    if (addr & ((1UL << GO_SHIFT_BITS) - 1)) return; // unaligned

    size_t primary_idx = (addr >> (GO_SECONDARY_BITS + GO_SHIFT_BITS)) & (GO_PRIMARY_ENTRIES - 1);
    size_t secondary_idx = (addr >> GO_SHIFT_BITS) & (GO_SECONDARY_ENTRIES - 1);

    go_grade_t* st = get_secondary_table(primary_idx);
    // Non-atomic store of 48-byte record (potential torn metadata if concurrent)
    st[secondary_idx] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    uintptr_t addr = (uintptr_t)slot_addr;
    if (addr & ((1UL << GO_SHIFT_BITS) - 1)) return get_top();

    size_t primary_idx = (addr >> (GO_SECONDARY_BITS + GO_SHIFT_BITS)) & (GO_PRIMARY_ENTRIES - 1);
    size_t secondary_idx = (addr >> GO_SHIFT_BITS) & (GO_SECONDARY_ENTRIES - 1);

    atomic_secondary_ptr_t* pt = atomic_load_explicit(&primary_table, memory_order_acquire);
    if (!pt) return get_top();

    go_grade_t* st = atomic_load_explicit(&pt[primary_idx], memory_order_acquire);
    if (!st) return get_top();

    go_grade_t g = st[secondary_idx];
    // Return TOP if entry is zeroed (uninitialized)
    if (g.base == 0 && g.end == 0) return get_top();
    return g;
}
