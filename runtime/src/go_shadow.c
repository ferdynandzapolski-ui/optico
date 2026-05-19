#include "goirrt.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <assert.h>

#ifndef MAP_NORESERVE
#define MAP_NORESERVE 0x4000
#endif

/*
 * Disjoint Metadata Shadow Store (Two-Level Table)
 * Mapping: [47 ... 26] (22 bits) -> Primary Index
 *          [25 ...  3] (23 bits) -> Secondary Index
 * Supports 48-bit virtual address space with 8-byte pointer alignment.
 */

#define PRIMARY_BITS 22
#define SECONDARY_BITS 23
#define SHIFT 3

#define PRIMARY_SIZE (1ULL << PRIMARY_BITS)
#define SECONDARY_SIZE (1ULL << SECONDARY_BITS)

#define PRIMARY_MASK (PRIMARY_SIZE - 1)
#define SECONDARY_MASK (SECONDARY_SIZE - 1)

static go_grade_t **primary_table = NULL;

__attribute__((constructor))
static void __go_shadow_init(void) {
    size_t size = PRIMARY_SIZE * sizeof(go_grade_t *);
    primary_table = (go_grade_t **)mmap(NULL, size, PROT_READ | PROT_WRITE,
                                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (primary_table == MAP_FAILED) {
        primary_table = NULL;
    }
}

static go_grade_t *__go_get_secondary(uintptr_t addr) {
    if (!primary_table) return NULL;

    uintptr_t p_idx = (addr >> (SECONDARY_BITS + SHIFT)) & PRIMARY_MASK;
    go_grade_t *secondary = __atomic_load_n(&primary_table[p_idx], __ATOMIC_ACQUIRE);

    if (secondary == NULL) {
        size_t size = SECONDARY_SIZE * sizeof(go_grade_t);
        void *new_table = mmap(NULL, size, PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (new_table == MAP_FAILED) return NULL;

        go_grade_t *expected = NULL;
        if (!__atomic_compare_exchange_n(&primary_table[p_idx], &expected, (go_grade_t *)new_table,
                                         false, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE)) {
            munmap(new_table, size);
            secondary = expected;
        } else {
            secondary = (go_grade_t *)new_table;
        }
    }
    return secondary;
}

void __go_shadow_store(void *slot_addr, go_grade_t g) {
    uintptr_t addr = (uintptr_t)slot_addr;
    assert((addr & 0x7) == 0 && "Slot address must be 8-byte aligned");

    go_grade_t *secondary = __go_get_secondary(addr);
    if (!secondary) return;

    uintptr_t s_idx = (addr >> SHIFT) & SECONDARY_MASK;
    secondary[s_idx] = g;
}

go_grade_t __go_shadow_load(void *slot_addr) {
    uintptr_t addr = (uintptr_t)slot_addr;
    // We don't assert alignment here to allow conservative fallback for unaligned loads
    if (addr & 0x7) {
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }

    uintptr_t p_idx = (addr >> (SECONDARY_BITS + SHIFT)) & PRIMARY_MASK;
    if (!primary_table) {
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }

    go_grade_t *secondary = __atomic_load_n(&primary_table[p_idx], __ATOMIC_ACQUIRE);
    if (!secondary) {
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }

    uintptr_t s_idx = (addr >> SHIFT) & SECONDARY_MASK;
    go_grade_t g = secondary[s_idx];

    // Default to TOP if the entry is zeroed out (uninitialized)
    if (g.end == 0 && g.base == 0) {
        g.end = -1ULL;
        g.perms = 0xF;
    }

    return g;
}
