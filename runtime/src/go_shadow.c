#include "go_shadow.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define PRIMARY_BITS 22
#define SECONDARY_BITS 23

#define PRIMARY_SIZE (1UL << PRIMARY_BITS)
#define SECONDARY_SIZE (1UL << SECONDARY_BITS)

static go_grade_t** primary_table = NULL;

static go_grade_t get_top_grade() {
    go_grade_t g = {0};
    g.end = -1ULL;
    g.perms = 0xF;
    return g;
}

__attribute__((constructor))
static void __go_shadow_init() {
    primary_table = (go_grade_t**)mmap(NULL, PRIMARY_SIZE * sizeof(go_grade_t*),
                                       PROT_READ | PROT_WRITE,
                                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (primary_table == MAP_FAILED) {
        perror("GOIR: Failed to allocate primary shadow table");
        abort();
    }
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    uintptr_t addr = (uintptr_t)slot_addr;
    assert((addr & 0x7) == 0 && "GOIR: Shadow store address must be 8-byte aligned");

    uintptr_t p_idx = (addr >> 26) & (PRIMARY_SIZE - 1);
    uintptr_t s_idx = (addr >> 3) & (SECONDARY_SIZE - 1);

    if (primary_table[p_idx] == NULL) {
        go_grade_t* s_table = (go_grade_t*)mmap(NULL, SECONDARY_SIZE * sizeof(go_grade_t),
                                               PROT_READ | PROT_WRITE,
                                               MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (s_table == MAP_FAILED) {
            perror("GOIR: Failed to allocate secondary shadow table");
            abort();
        }

        if (!__sync_bool_compare_and_swap(&primary_table[p_idx], NULL, s_table)) {
            // Another thread allocated it first, free our redundant table
            munmap(s_table, SECONDARY_SIZE * sizeof(go_grade_t));
        }
    }

    primary_table[p_idx][s_idx] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    uintptr_t addr = (uintptr_t)slot_addr;
    // For load, we might be more lenient or just assert as well.
    // Given the design, application pointers holding pointers MUST be aligned.
    assert((addr & 0x7) == 0 && "GOIR: Shadow load address must be 8-byte aligned");

    uintptr_t p_idx = (addr >> 26) & (PRIMARY_SIZE - 1);
    uintptr_t s_idx = (addr >> 3) & (SECONDARY_SIZE - 1);

    if (primary_table[p_idx] == NULL) {
        return get_top_grade();
    }

    return primary_table[p_idx][s_idx];
}
