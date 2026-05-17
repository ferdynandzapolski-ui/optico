#include "go_shadow.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#define PRIMARY_BITS 22
#define SECONDARY_BITS 23
#define PRIMARY_SIZE (1ULL << PRIMARY_BITS)
#define SECONDARY_SIZE (1ULL << SECONDARY_BITS)

static go_grade_t** primary_table = NULL;

static void __go_shadow_init(void) __attribute__((constructor));

static void __go_shadow_init(void) {
    size_t size = PRIMARY_SIZE * sizeof(go_grade_t*);
    primary_table = (go_grade_t**)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (primary_table == MAP_FAILED) {
        perror("mmap primary_table");
        exit(1);
    }
}

static go_grade_t __go_get_top_grade(void) {
    go_grade_t g;
    memset(&g, 0, sizeof(g));
    g.end = -1ULL;
    g.perms = 0xF;
    return g;
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    uint64_t v = (uint64_t)slot_addr;
    assert((v & 0x7) == 0 && "slot_addr must be 8-byte aligned");

    uint64_t p_idx = (v >> 26) & 0x3FFFFF;
    uint64_t s_idx = (v >> 3) & 0x7FFFFF;

    if (p_idx >= PRIMARY_SIZE) return;

    go_grade_t* secondary = __atomic_load_n(&primary_table[p_idx], __ATOMIC_ACQUIRE);
    if (!secondary) {
        size_t size = SECONDARY_SIZE * sizeof(go_grade_t);
        go_grade_t* new_secondary = (go_grade_t*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (new_secondary == MAP_FAILED) {
            perror("mmap secondary_table");
            exit(1);
        }

        go_grade_t* expected = NULL;
        if (!__atomic_compare_exchange_n(&primary_table[p_idx], &expected, new_secondary, false, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE)) {
            munmap(new_secondary, size);
            secondary = expected;
        } else {
            secondary = new_secondary;
        }
    }

    secondary[s_idx] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    uint64_t v = (uint64_t)slot_addr;
    assert((v & 0x7) == 0 && "slot_addr must be 8-byte aligned");

    uint64_t p_idx = (v >> 26) & 0x3FFFFF;
    uint64_t s_idx = (v >> 3) & 0x7FFFFF;

    if (p_idx >= PRIMARY_SIZE) return __go_get_top_grade();

    go_grade_t* secondary = __atomic_load_n(&primary_table[p_idx], __ATOMIC_ACQUIRE);
    if (!secondary) {
        return __go_get_top_grade();
    }

    go_grade_t g = secondary[s_idx];
    // If base is 0 and end is 0 (except for top), it might be uninitialized
    // In our top grade, base is 0 and end is -1.
    if (g.base == 0 && g.end == 0) {
        return __go_get_top_grade();
    }

    return g;
}
