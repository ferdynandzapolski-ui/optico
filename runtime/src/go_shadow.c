#include "go_shadow.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <assert.h>
#include <stdatomic.h>
#include <string.h>

#define PRIMARY_BITS 22
#define SECONDARY_BITS 23
#define SHIFT 3

#define PRIMARY_SIZE (1ULL << PRIMARY_BITS)
#define SECONDARY_SIZE (1ULL << SECONDARY_BITS)

typedef go_grade_t* secondary_table_t;
static secondary_table_t* primary_table = NULL;

__attribute__((constructor))
static void __go_shadow_init() {
    size_t size = PRIMARY_SIZE * sizeof(secondary_table_t);
    primary_table = (secondary_table_t*)mmap(NULL, size, PROT_READ | PROT_WRITE,
                                            MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
                                            -1, 0);
    if (primary_table == MAP_FAILED) {
        primary_table = NULL;
    }
}

static inline go_grade_t __go_get_top_grade() {
    go_grade_t g = {0};
    g.end = -1ULL;
    g.perms = 0xF; // R=1, W=2, F=4, X=8
    return g;
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    if (!primary_table) return;

    // Ensure 8-byte alignment for the slot address
    assert(((uintptr_t)slot_addr & 0x7) == 0);

    uintptr_t addr = (uintptr_t)slot_addr;
    uintptr_t p_idx = (addr >> (SECONDARY_BITS + SHIFT)) & (PRIMARY_SIZE - 1);
    uintptr_t s_idx = (addr >> SHIFT) & (SECONDARY_SIZE - 1);

    secondary_table_t s_table = atomic_load_explicit((_Atomic(secondary_table_t)*)&primary_table[p_idx], memory_order_acquire);
    if (!s_table) {
        size_t size = SECONDARY_SIZE * sizeof(go_grade_t);
        secondary_table_t new_table = (secondary_table_t)mmap(NULL, size, PROT_READ | PROT_WRITE,
                                                             MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
                                                             -1, 0);
        if (new_table == MAP_FAILED) return;

        secondary_table_t expected = NULL;
        if (!atomic_compare_exchange_strong_explicit((_Atomic(secondary_table_t)*)&primary_table[p_idx],
                                                    &expected, new_table,
                                                    memory_order_release, memory_order_acquire)) {
            munmap(new_table, size);
            s_table = expected;
        } else {
            s_table = new_table;
        }
    }

    s_table[s_idx] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    if (!primary_table || ((uintptr_t)slot_addr & 0x7) != 0) {
        return __go_get_top_grade();
    }

    uintptr_t addr = (uintptr_t)slot_addr;
    uintptr_t p_idx = (addr >> (SECONDARY_BITS + SHIFT)) & (PRIMARY_SIZE - 1);
    uintptr_t s_idx = (addr >> SHIFT) & (SECONDARY_SIZE - 1);

    secondary_table_t s_table = atomic_load_explicit((_Atomic(secondary_table_t)*)&primary_table[p_idx], memory_order_acquire);
    if (!s_table) {
        return __go_get_top_grade();
    }

    go_grade_t g = s_table[s_idx];
    // If the entry is all zeros, it was never written.
    // However, base=0 might be valid for some null-derived pointers in some dialects.
    // In GOIR, we assume a zeroed entry means TOP grade for safety/conservatism if uninitialized.
    // Wait, if it's zeroed, perms=0 which is more restrictive.
    // The spec says uninitialized slots return TOP.
    if (g.base == 0 && g.end == 0 && g.perms == 0) {
        return __go_get_top_grade();
    }

    return g;
}
