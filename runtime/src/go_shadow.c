#include "go_shadow.h"
#include <sys/mman.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#ifndef MAP_NORESERVE
#define MAP_NORESERVE 0
#endif

typedef _Atomic(go_grade_t*) atomic_secondary_ptr_t;

static _Atomic(atomic_secondary_ptr_t*) primary_table = NULL;

void __go_shadow_init(void) {
    if (atomic_load_explicit(&primary_table, memory_order_acquire)) return;

    size_t primary_size = GO_PRIMARY_SIZE * sizeof(atomic_secondary_ptr_t);
    void* p = mmap(NULL, primary_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (p == MAP_FAILED) {
        perror("GOIR Shadow: Failed to allocate primary table");
        abort();
    }

    atomic_secondary_ptr_t* expected = NULL;
    if (!atomic_compare_exchange_strong_explicit(&primary_table, &expected, (atomic_secondary_ptr_t*)p, memory_order_acq_rel, memory_order_acquire)) {
        munmap(p, primary_size);
    }
}

static go_grade_t* get_secondary_table(uintptr_t primary_idx) {
    atomic_secondary_ptr_t* prim = atomic_load_explicit(&primary_table, memory_order_acquire);
    if (!prim) {
        __go_shadow_init();
        prim = atomic_load_explicit(&primary_table, memory_order_acquire);
    }

    go_grade_t* secondary = atomic_load_explicit(&prim[primary_idx], memory_order_acquire);
    if (secondary) return secondary;

    size_t secondary_size = GO_SECONDARY_SIZE * sizeof(go_grade_t);
    void* p = mmap(NULL, secondary_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (p == MAP_FAILED) {
        perror("GOIR Shadow: Failed to allocate secondary table");
        abort();
    }

    go_grade_t* expected = NULL;
    if (atomic_compare_exchange_strong_explicit(&prim[primary_idx], &expected, (go_grade_t*)p, memory_order_acq_rel, memory_order_acquire)) {
        return (go_grade_t*)p;
    } else {
        munmap(p, secondary_size);
        return expected;
    }
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    uintptr_t addr = (uintptr_t)slot_addr;
    if (addr & 0x7) return; // 8-byte alignment required

    uintptr_t shifted_addr = addr >> GO_SHIFT_BITS;
    uintptr_t secondary_idx = shifted_addr & (GO_SECONDARY_SIZE - 1);
    uintptr_t primary_idx = shifted_addr >> GO_SECONDARY_BITS;

    if (primary_idx >= GO_PRIMARY_SIZE) return;

    go_grade_t* secondary = get_secondary_table(primary_idx);
    secondary[secondary_idx] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    uintptr_t addr = (uintptr_t)slot_addr;
    go_grade_t g_top = {0};
    g_top.end = -1ULL;
    g_top.perms = 0xF;

    if (addr & 0x7) return g_top;

    uintptr_t shifted_addr = addr >> GO_SHIFT_BITS;
    uintptr_t secondary_idx = shifted_addr & (GO_SECONDARY_SIZE - 1);
    uintptr_t primary_idx = shifted_addr >> GO_SECONDARY_BITS;

    if (primary_idx >= GO_PRIMARY_SIZE) return g_top;

    atomic_secondary_ptr_t* prim = atomic_load_explicit(&primary_table, memory_order_acquire);
    if (!prim) return g_top;

    go_grade_t* secondary = atomic_load_explicit(&prim[primary_idx], memory_order_acquire);
    if (!secondary) return g_top;

    go_grade_t g = secondary[secondary_idx];
    // Return TOP if entry is uninitialized (base == 0 and end == 0)
    if (g.base == 0 && g.end == 0) return g_top;
    return g;
}
