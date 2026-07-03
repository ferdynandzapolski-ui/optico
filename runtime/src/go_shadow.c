#include "go_shadow.h"
#include <sys/mman.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PRIMARY_SIZE (1UL << GO_PRIMARY_BITS)
#define SECONDARY_SIZE (1UL << GO_SECONDARY_BITS)

typedef _Atomic(go_grade_t*) atomic_secondary_ptr_t;

static _Atomic(atomic_secondary_ptr_t*) primary_table = NULL;

void __go_shadow_init(void) {
    if (atomic_load_explicit(&primary_table, memory_order_acquire) != NULL) {
        return;
    }

    size_t size = PRIMARY_SIZE * sizeof(atomic_secondary_ptr_t);
    void* p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_NORESERVE, -1, 0);
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
    atomic_secondary_ptr_t* table = atomic_load_explicit(&primary_table, memory_order_acquire);
    if (!table) {
        __go_shadow_init();
        table = atomic_load_explicit(&primary_table, memory_order_acquire);
    }

    go_grade_t* secondary = atomic_load_explicit(&table[primary_idx], memory_order_acquire);
    if (secondary == NULL) {
        size_t size = SECONDARY_SIZE * sizeof(go_grade_t);
        void* p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_NORESERVE, -1, 0);
        if (p == MAP_FAILED) {
            perror("GOIR Shadow: Failed to mmap secondary table");
            abort();
        }

        go_grade_t* expected = NULL;
        if (!atomic_compare_exchange_strong_explicit(&table[primary_idx], &expected, (go_grade_t*)p,
                                                    memory_order_acq_rel, memory_order_acquire)) {
            munmap(p, size);
            secondary = expected;
        } else {
            secondary = (go_grade_t*)p;
        }
    }
    return secondary;
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    uintptr_t addr = (uintptr_t)slot_addr;
    if (addr & ((1UL << GO_SHIFT_BITS) - 1)) {
        return; // Only 8-byte aligned slots supported
    }

    uintptr_t shifted = addr >> GO_SHIFT_BITS;
    uintptr_t primary_idx = shifted >> GO_SECONDARY_BITS;
    uintptr_t secondary_idx = shifted & (SECONDARY_SIZE - 1);

    if (primary_idx >= PRIMARY_SIZE) return;

    go_grade_t* secondary = get_secondary_table(primary_idx);
    secondary[secondary_idx] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    uintptr_t addr = (uintptr_t)slot_addr;
    go_grade_t g_top = {0};
    g_top.end = -1ULL;
    g_top.perms = 0xF;

    if (addr & ((1UL << GO_SHIFT_BITS) - 1)) {
        return g_top;
    }

    uintptr_t shifted = addr >> GO_SHIFT_BITS;
    uintptr_t primary_idx = shifted >> GO_SECONDARY_BITS;
    uintptr_t secondary_idx = shifted & (SECONDARY_SIZE - 1);

    if (primary_idx >= PRIMARY_SIZE) return g_top;

    atomic_secondary_ptr_t* table = atomic_load_explicit(&primary_table, memory_order_acquire);
    if (!table) return g_top;

    go_grade_t* secondary = atomic_load_explicit(&table[primary_idx], memory_order_acquire);
    if (!secondary) return g_top;

    go_grade_t g = secondary[secondary_idx];
    if (g.base == 0 && g.end == 0) return g_top; // Uninitialized
    return g;
}
