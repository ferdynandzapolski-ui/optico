#include "go_shadow.h"
#include <sys/mman.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#define PRIMARY_BITS 22
#define SECONDARY_BITS 23
#define PRIMARY_SIZE (1ULL << PRIMARY_BITS)
#define SECONDARY_SIZE (1ULL << SECONDARY_BITS)

// Primary table maps bits [47:26] of address
static _Atomic(go_grade_t*) *primary_table = NULL;

static void __attribute__((constructor)) __go_shadow_init() {
    void* mem = mmap(NULL, PRIMARY_SIZE * sizeof(void*), PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (mem == MAP_FAILED) {
        perror("GOIR Shadow: Failed to allocate primary table");
        abort();
    }
    primary_table = (_Atomic(go_grade_t*) *)mem;
}

static go_grade_t* __go_get_secondary(void* slot_addr, bool create) {
    uintptr_t addr = (uintptr_t)slot_addr;
    // Primary index uses bits 47 down to 26
    uintptr_t p_idx = (addr >> 26) & (PRIMARY_SIZE - 1);

    go_grade_t* secondary = atomic_load_explicit(&primary_table[p_idx], memory_order_acquire);
    if (secondary == NULL) {
        if (!create) return NULL;

        size_t size = SECONDARY_SIZE * sizeof(go_grade_t);
        void* new_sec = mmap(NULL, size, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (new_sec == MAP_FAILED) {
            perror("GOIR Shadow: Failed to allocate secondary table");
            abort();
        }

        go_grade_t* expected = NULL;
        if (!atomic_compare_exchange_strong_explicit(&primary_table[p_idx], &expected,
                                                     (go_grade_t*)new_sec,
                                                     memory_order_release,
                                                     memory_order_acquire)) {
            munmap(new_sec, size);
            secondary = expected;
        } else {
            secondary = (go_grade_t*)new_sec;
        }
    }
    return secondary;
}

static go_grade_t __go_get_top_grade() {
    go_grade_t g = {0};
    g.base = 0;
    g.end = -1ULL;
    g.perms = 0xF;
    return g;
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    // Shadow metadata is tracked for pointer-sized slots.
    // We assume 8-byte alignment for these slots.
    assert(((uintptr_t)slot_addr & 7) == 0);

    go_grade_t* secondary = __go_get_secondary(slot_addr, true);
    // Secondary index uses bits 25 down to 3
    uintptr_t s_idx = ((uintptr_t)slot_addr >> 3) & (SECONDARY_SIZE - 1);
    secondary[s_idx] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    if (((uintptr_t)slot_addr & 7) != 0) {
        return __go_get_top_grade();
    }

    go_grade_t* secondary = __go_get_secondary(slot_addr, false);
    if (secondary == NULL) {
        return __go_get_top_grade();
    }

    uintptr_t s_idx = ((uintptr_t)slot_addr >> 3) & (SECONDARY_SIZE - 1);
    go_grade_t g = secondary[s_idx];

    // If the entry is all zeros (uninitialized), return TOP
    if (g.end == 0 && g.perms == 0) {
        return __go_get_top_grade();
    }

    return g;
}
