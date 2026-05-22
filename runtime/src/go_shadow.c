#include "goirrt.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#ifndef MAP_NORESERVE
#define MAP_NORESERVE 0
#endif

/*
 * Disjoint Shadow Metadata Store: Two-level Table Architecture
 * Supports 48-bit address space.
 * 8-byte alignment (3-bit shift).
 * Primary Index: 22 bits (47-26)
 * Secondary Index: 23 bits (25-3)
 */

#define PRIMARY_BITS 22
#define SECONDARY_BITS 23
#define ALIGN_SHIFT 3

#define PRIMARY_SIZE (1ULL << PRIMARY_BITS)
#define SECONDARY_SIZE (1ULL << SECONDARY_BITS)

static go_grade_t** primary_table = NULL;

__attribute__((constructor))
static void __go_shadow_init(void) {
    size_t p_size = PRIMARY_SIZE * sizeof(go_grade_t*);
    primary_table = (go_grade_t**)mmap(NULL, p_size, PROT_READ | PROT_WRITE,
                                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (primary_table == MAP_FAILED) {
        perror("GOIR: Failed to mmap primary shadow table");
        exit(1);
    }
}

static go_grade_t* __go_shadow_get_secondary(uintptr_t addr) {
    uintptr_t p_idx = (addr >> (SECONDARY_BITS + ALIGN_SHIFT)) & (PRIMARY_SIZE - 1);
    go_grade_t* secondary = __atomic_load_n(&primary_table[p_idx], __ATOMIC_RELAXED);

    if (!secondary) {
        size_t s_size = SECONDARY_SIZE * sizeof(go_grade_t);
        go_grade_t* new_secondary = (go_grade_t*)mmap(NULL, s_size, PROT_READ | PROT_WRITE,
                                                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (new_secondary == MAP_FAILED) {
            perror("GOIR: Failed to mmap secondary shadow table");
            return NULL;
        }

        if (!__atomic_compare_exchange_n(&primary_table[p_idx], &secondary, new_secondary,
                                         false, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE)) {
            munmap(new_secondary, s_size);
        } else {
            secondary = new_secondary;
        }
    }
    return secondary;
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    uintptr_t addr = (uintptr_t)slot_addr;
    assert((addr & ((1 << ALIGN_SHIFT) - 1)) == 0 && "Shadow slot must be 8-byte aligned");

    go_grade_t* secondary = __go_shadow_get_secondary(addr);
    if (!secondary) return;

    uintptr_t s_idx = (addr >> ALIGN_SHIFT) & (SECONDARY_SIZE - 1);
    secondary[s_idx] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    uintptr_t addr = (uintptr_t)slot_addr;
    if ((addr & ((1 << ALIGN_SHIFT) - 1)) != 0) {
        return __go_get_top_grade();
    }

    uintptr_t p_idx = (addr >> (SECONDARY_BITS + ALIGN_SHIFT)) & (PRIMARY_SIZE - 1);
    go_grade_t* secondary = __atomic_load_n(&primary_table[p_idx], __ATOMIC_RELAXED);

    if (!secondary) {
        return __go_get_top_grade();
    }

    uintptr_t s_idx = (addr >> ALIGN_SHIFT) & (SECONDARY_SIZE - 1);
    go_grade_t g = secondary[s_idx];

    // If all zero (uninitialized secondary entry), return TOP
    if (g.base == 0 && g.end == 0 && g.perms == 0) {
        return __go_get_top_grade();
    }

    return g;
}
