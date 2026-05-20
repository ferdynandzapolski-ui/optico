#define _GNU_SOURCE
#include "go_shadow.h"
#include <sys/mman.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

/**
 * 48-bit Virtual Address Space Mapping for Shadow Metadata
 *
 * [47 ... 26] [25 ... 3] [2 ... 0]
 *   Primary     Secondary    Offset (8B aligned)
 *   22 bits     23 bits      3 bits
 */

#define PRIMARY_BITS 22
#define SECONDARY_BITS 23
#define SHIFT_BITS 3

#define PRIMARY_SIZE (1ULL << PRIMARY_BITS)
#define SECONDARY_SIZE (1ULL << SECONDARY_BITS)

#define PRIMARY_MASK (PRIMARY_SIZE - 1)
#define SECONDARY_MASK (SECONDARY_SIZE - 1)

static go_grade_t** primary_table = NULL;

__attribute__((constructor))
static void __go_shadow_init(void) {
    size_t size = PRIMARY_SIZE * sizeof(go_grade_t*);
    primary_table = (go_grade_t**)mmap(NULL, size, PROT_READ | PROT_WRITE,
                                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (primary_table == MAP_FAILED) {
        perror("GOIR: Failed to mmap primary shadow table");
        abort();
    }
}

static go_grade_t* __go_shadow_get_secondary(size_t p_idx) {
    go_grade_t* secondary = primary_table[p_idx];
    if (secondary == NULL) {
        size_t size = SECONDARY_SIZE * sizeof(go_grade_t);
        go_grade_t* new_secondary = (go_grade_t*)mmap(NULL, size, PROT_READ | PROT_WRITE,
                                                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (new_secondary == MAP_FAILED) {
            perror("GOIR: Failed to mmap secondary shadow table");
            abort();
        }

        go_grade_t* expected = NULL;
        if (!__atomic_compare_exchange_n(&primary_table[p_idx], &expected, new_secondary,
                                          false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
            // Someone else allocated it first
            munmap(new_secondary, size);
            return primary_table[p_idx];
        }
        return new_secondary;
    }
    return secondary;
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    uintptr_t addr = (uintptr_t)slot_addr;
    assert((addr & 0x7) == 0 && "Shadow slot address must be 8-byte aligned");

    size_t p_idx = (addr >> (SECONDARY_BITS + SHIFT_BITS)) & PRIMARY_MASK;
    size_t s_idx = (addr >> SHIFT_BITS) & SECONDARY_MASK;

    go_grade_t* secondary = __go_shadow_get_secondary(p_idx);
    secondary[s_idx] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    uintptr_t addr = (uintptr_t)slot_addr;
    if ((addr & 0x7) != 0) {
        // Return TOP for unaligned loads for robustness in diagnostic mode
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }

    size_t p_idx = (addr >> (SECONDARY_BITS + SHIFT_BITS)) & PRIMARY_MASK;
    size_t s_idx = (addr >> SHIFT_BITS) & SECONDARY_MASK;

    go_grade_t* secondary = primary_table[p_idx];
    if (secondary == NULL) {
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }

    go_grade_t g = secondary[s_idx];
    // If the entry is zeroed out, it means it's uninitialized (TOP)
    if (g.base == 0 && g.end == 0) {
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }
    return g;
}
