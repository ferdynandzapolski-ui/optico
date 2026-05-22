#define _GNU_SOURCE
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "go_shadow.h"

// 48-bit address space, 8-byte aligned slots.
// (addr >> 3) gives 45 bits.
// Split: 22 bits primary, 23 bits secondary.
#define PRIMARY_BITS 22
#define SECONDARY_BITS 23
#define PRIMARY_SIZE (1ULL << PRIMARY_BITS)
#define SECONDARY_SIZE (1ULL << SECONDARY_BITS)

static go_grade_t** primary_table = NULL;

static void __go_shadow_init(void) __attribute__((constructor));
static void __go_shadow_init(void) {
    primary_table = (go_grade_t**)mmap(NULL, PRIMARY_SIZE * sizeof(go_grade_t*),
                                       PROT_READ | PROT_WRITE,
                                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (primary_table == MAP_FAILED) {
        perror("Failed to mmap shadow primary table");
        exit(1);
    }
}

static go_grade_t __go_get_top_grade() {
    go_grade_t g = {0};
    g.end = -1ULL;
    g.perms = 0xF;
    return g;
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    uintptr_t addr = (uintptr_t)slot_addr;
    assert((addr & 7) == 0 && "slot_addr must be 8-byte aligned");

    uintptr_t idx = addr >> 3;
    uintptr_t p_idx = idx >> SECONDARY_BITS;
    uintptr_t s_idx = idx & (SECONDARY_SIZE - 1);

    if (p_idx >= PRIMARY_SIZE) return;

    go_grade_t* secondary = __atomic_load_n(&primary_table[p_idx], __ATOMIC_ACQUIRE);
    if (secondary == NULL) {
        size_t sec_bytes = SECONDARY_SIZE * sizeof(go_grade_t);
        go_grade_t* new_secondary = (go_grade_t*)mmap(NULL, sec_bytes,
                                                     PROT_READ | PROT_WRITE,
                                                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (new_secondary == MAP_FAILED) {
            perror("Failed to mmap shadow secondary table");
            exit(1);
        }
        go_grade_t* expected = NULL;
        if (!__atomic_compare_exchange_n(&primary_table[p_idx], &expected, new_secondary, false, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE)) {
            munmap(new_secondary, sec_bytes);
            secondary = expected;
        } else {
            secondary = new_secondary;
        }
    }

    // NOTE: Non-atomic assignment of 48-byte struct. Metadata may be "torn" if
    // concurrent stores to the same slot occur without external synchronization.
    secondary[s_idx] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    uintptr_t addr = (uintptr_t)slot_addr;
    if ((addr & 7) != 0) {
        return __go_get_top_grade();
    }

    uintptr_t idx = addr >> 3;
    uintptr_t p_idx = idx >> SECONDARY_BITS;
    uintptr_t s_idx = idx & (SECONDARY_SIZE - 1);

    if (p_idx >= PRIMARY_SIZE) {
        return __go_get_top_grade();
    }

    go_grade_t* secondary = __atomic_load_n(&primary_table[p_idx], __ATOMIC_ACQUIRE);
    if (secondary == NULL) {
        return __go_get_top_grade();
    }

    go_grade_t g = secondary[s_idx];
    // If all zero, assume uninitialized and return TOP to avoid false positives.
    if (g.base == 0 && g.end == 0 && g.perms == 0) {
        return __go_get_top_grade();
    }
    return g;
}
