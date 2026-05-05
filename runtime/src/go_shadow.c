#include "go_shadow.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <stdatomic.h>

/*
 * Two-level shadow table design for 48-bit address space (parameterized).
 * Assuming 8-byte aligned pointer slots.
 */

#ifndef ADDR_BITS
#define ADDR_BITS 48
#endif

#define ALIGN_BITS 3
#define TOTAL_INDEX_BITS (ADDR_BITS - ALIGN_BITS)

#define PRIMARY_BITS (TOTAL_INDEX_BITS / 2)
#define SECONDARY_BITS (TOTAL_INDEX_BITS - PRIMARY_BITS)

#define PRIMARY_ENTRIES (1UL << PRIMARY_BITS)
#define SECONDARY_ENTRIES (1UL << SECONDARY_BITS)

typedef go_grade_t* secondary_table_t;

static _Atomic(secondary_table_t)* primary_table = NULL;

static void go_shadow_init() __attribute__((constructor));

static void go_shadow_init() {
    if (primary_table) return;
    size_t size = PRIMARY_ENTRIES * sizeof(_Atomic(secondary_table_t));
    _Atomic(secondary_table_t)* pt = (_Atomic(secondary_table_t)*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (pt == MAP_FAILED) {
        perror("GOIR: failed to allocate primary shadow table");
        abort();
    }

    _Atomic(secondary_table_t)* expected = NULL;
    if (!atomic_compare_exchange_strong(&primary_table, &expected, pt)) {
        munmap(pt, size);
    }
}

static go_grade_t get_top_grade() {
    go_grade_t g = {0};
    g.end = -1ULL;
    g.perms = 0xF;
    return g;
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    if (!primary_table) go_shadow_init();

    uintptr_t addr = (uintptr_t)slot_addr;
    uintptr_t idx1 = (addr >> (SECONDARY_BITS + ALIGN_BITS)) & (PRIMARY_ENTRIES - 1);
    uintptr_t idx2 = (addr >> ALIGN_BITS) & (SECONDARY_ENTRIES - 1);

    secondary_table_t sec = atomic_load(&primary_table[idx1]);
    if (sec == NULL) {
        size_t size = SECONDARY_ENTRIES * sizeof(go_grade_t);
        secondary_table_t new_sec = (secondary_table_t)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (new_sec == MAP_FAILED) {
            perror("GOIR: failed to allocate secondary shadow table");
            abort();
        }

        secondary_table_t expected = NULL;
        if (!atomic_compare_exchange_strong(&primary_table[idx1], &expected, new_sec)) {
            munmap(new_sec, size);
            sec = expected;
        } else {
            sec = new_sec;
        }
    }

    sec[idx2] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    if (!primary_table) return get_top_grade();

    uintptr_t addr = (uintptr_t)slot_addr;
    uintptr_t idx1 = (addr >> (SECONDARY_BITS + ALIGN_BITS)) & (PRIMARY_ENTRIES - 1);
    uintptr_t idx2 = (addr >> ALIGN_BITS) & (SECONDARY_ENTRIES - 1);

    secondary_table_t sec = atomic_load(&primary_table[idx1]);
    if (sec == NULL) return get_top_grade();

    return sec[idx2];
}
