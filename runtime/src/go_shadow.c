#include "go_shadow.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

/*
 * Shadow table parameters:
 * Assuming a 48-bit virtual address space.
 * Alignment: 8 bytes (3 bits)
 * Primary index: 22 bits
 * Secondary index: 23 bits
 * 22 + 23 + 3 = 48 bits
 */
#define PRIMARY_BITS 22
#define SECONDARY_BITS 23
#define PRIMARY_SIZE (1ULL << PRIMARY_BITS)
#define SECONDARY_SIZE (1ULL << SECONDARY_BITS)
#define ALIGN_SHIFT 3

typedef go_grade_t* secondary_table_t;

static secondary_table_t* primary_table = NULL;

static void __attribute__((constructor)) __go_shadow_init() {
    size_t table_size = PRIMARY_SIZE * sizeof(secondary_table_t);
    primary_table = (secondary_table_t*)mmap(NULL, table_size, PROT_READ | PROT_WRITE,
                                            MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    assert(primary_table != MAP_FAILED && "Failed to mmap shadow primary table");
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    assert(((uintptr_t)slot_addr & ((1 << ALIGN_SHIFT) - 1)) == 0 && "Unaligned shadow store");

    uintptr_t addr = (uintptr_t)slot_addr;
    uintptr_t p_idx = (addr >> (SECONDARY_BITS + ALIGN_SHIFT)) & (PRIMARY_SIZE - 1);
    uintptr_t s_idx = (addr >> ALIGN_SHIFT) & (SECONDARY_SIZE - 1);

    secondary_table_t st = __atomic_load_n(&primary_table[p_idx], __ATOMIC_ACQUIRE);
    if (!st) {
        size_t st_size = SECONDARY_SIZE * sizeof(go_grade_t);
        secondary_table_t new_st = (secondary_table_t)mmap(NULL, st_size, PROT_READ | PROT_WRITE,
                                                          MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        assert(new_st != MAP_FAILED && "Failed to mmap shadow secondary table");

        secondary_table_t expected = NULL;
        if (!__atomic_compare_exchange_n(&primary_table[p_idx], &expected, new_st, false, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE)) {
            munmap(new_st, st_size);
            st = expected;
        } else {
            st = new_st;
        }
    }

    st[s_idx] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    uintptr_t addr = (uintptr_t)slot_addr;
    uintptr_t p_idx = (addr >> (SECONDARY_BITS + ALIGN_SHIFT)) & (PRIMARY_SIZE - 1);
    uintptr_t s_idx = (addr >> ALIGN_SHIFT) & (SECONDARY_SIZE - 1);

    secondary_table_t st = __atomic_load_n(&primary_table[p_idx], __ATOMIC_ACQUIRE);
    if (!st) {
        return __go_get_top_grade();
    }

    go_grade_t g = st[s_idx];
    // If grade is all zeros (uninitialized), return TOP.
    // Assuming base=0 and end=0 is uninitialized/invalid.
    if (g.base == 0 && g.end == 0) {
        return __go_get_top_grade();
    }

    return g;
}
