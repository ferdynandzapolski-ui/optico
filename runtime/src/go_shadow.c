#include "go_shadow.h"
#include <sys/mman.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/* Parameterized Address Space Decomposition (Defaults to 48-bit) */
#ifndef GO_PRIMARY_BITS
#define GO_PRIMARY_BITS 22
#endif

#ifndef GO_SECONDARY_BITS
#define GO_SECONDARY_BITS 23
#endif

#ifndef GO_SHIFT_BITS
#define GO_SHIFT_BITS 3
#endif

#define PRIMARY_SIZE (1ULL << GO_PRIMARY_BITS)
#define SECONDARY_SIZE (1ULL << GO_SECONDARY_BITS)

/*
 * Primary table is an array of atomic pointers to secondary tables.
 * Using a separate atomic pointer for the primary table itself to handle lazy init.
 */
typedef _Atomic(go_grade_t*) atomic_s_table_ptr;
static _Atomic(atomic_s_table_ptr*) primary_table = NULL;

static void __go_shadow_init() {
    if (atomic_load_explicit(&primary_table, memory_order_relaxed)) return;

    size_t size = PRIMARY_SIZE * sizeof(atomic_s_table_ptr);
    void* p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (p == MAP_FAILED) {
        perror("GOIR: Failed to mmap primary shadow table");
        abort();
    }

    atomic_s_table_ptr* expected = NULL;
    if (!atomic_compare_exchange_strong(&primary_table, &expected, (atomic_s_table_ptr*)p)) {
        munmap(p, size);
    }
}

static go_grade_t get_top_grade() {
    go_grade_t g = {0};
    g.end = -1ULL;
    g.perms = 0xF;
    return g;
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    if (((uintptr_t)slot_addr & ((1 << GO_SHIFT_BITS) - 1)) != 0) return;

    __go_shadow_init();

    uintptr_t addr = (uintptr_t)slot_addr;
    uintptr_t p_idx = (addr >> (GO_SECONDARY_BITS + GO_SHIFT_BITS)) & (PRIMARY_SIZE - 1);
    uintptr_t s_idx = (addr >> GO_SHIFT_BITS) & (SECONDARY_SIZE - 1);

    atomic_s_table_ptr* p_table = atomic_load_explicit(&primary_table, memory_order_acquire);
    go_grade_t* s_table = atomic_load_explicit(&p_table[p_idx], memory_order_acquire);

    if (!s_table) {
        size_t size = SECONDARY_SIZE * sizeof(go_grade_t);
        void* new_table = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (new_table == MAP_FAILED) {
            perror("GOIR: Failed to mmap secondary shadow table");
            abort();
        }

        go_grade_t* expected = NULL;
        if (!atomic_compare_exchange_strong(&p_table[p_idx], &expected, (go_grade_t*)new_table)) {
            munmap(new_table, size);
            s_table = expected;
        } else {
            s_table = (go_grade_t*)new_table;
        }
    }

    s_table[s_idx] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    if (((uintptr_t)slot_addr & ((1 << GO_SHIFT_BITS) - 1)) != 0) return get_top_grade();

    atomic_s_table_ptr* p_table = atomic_load_explicit(&primary_table, memory_order_acquire);
    if (!p_table) return get_top_grade();

    uintptr_t addr = (uintptr_t)slot_addr;
    uintptr_t p_idx = (addr >> (GO_SECONDARY_BITS + GO_SHIFT_BITS)) & (PRIMARY_SIZE - 1);
    uintptr_t s_idx = (addr >> GO_SHIFT_BITS) & (SECONDARY_SIZE - 1);

    go_grade_t* s_table = atomic_load_explicit(&p_table[p_idx], memory_order_acquire);
    if (!s_table) return get_top_grade();

    go_grade_t g = s_table[s_idx];
    if (g.base == 0 && g.end == 0) return get_top_grade();

    return g;
}
