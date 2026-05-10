#include "go_shadow.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

// 48-bit address space
#define ADDR_BITS 48
#define ALIGN_SHIFT 3
#define V_IDX_BITS (ADDR_BITS - ALIGN_SHIFT)

// Two-level table: 22 bits primary, 23 bits secondary
#define PRIMARY_BITS 22
#define SECONDARY_BITS 23

#define PRIMARY_SIZE (1ULL << PRIMARY_BITS)
#define SECONDARY_SIZE (1ULL << SECONDARY_BITS)

#define SECONDARY_BYTE_SIZE (SECONDARY_SIZE * sizeof(go_grade_t))

static go_grade_t** primary_table = NULL;

void __go_shadow_init(void) {
    if (primary_table) return;

    // Allocate primary table
    primary_table = (go_grade_t**)mmap(NULL, PRIMARY_SIZE * sizeof(void*),
                                       PROT_READ | PROT_WRITE,
                                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
                                       -1, 0);
    if (primary_table == MAP_FAILED) {
        perror("mmap primary_table");
        abort();
    }
}

// Ensure primary table is initialized
__attribute__((constructor))
static void shadow_ctor(void) {
    __go_shadow_init();
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    uintptr_t addr = (uintptr_t)slot_addr;
    assert((addr & 0x7) == 0 && "slot_addr must be 8-byte aligned");

    uintptr_t v_idx = addr >> ALIGN_SHIFT;
    uintptr_t p_idx = v_idx >> SECONDARY_BITS;
    uintptr_t s_idx = v_idx & (SECONDARY_SIZE - 1);

    if (p_idx >= PRIMARY_SIZE) {
        // Outside supported address space
        return;
    }

    if (primary_table[p_idx] == NULL) {
        // Allocate secondary table
        go_grade_t* s_table = (go_grade_t*)mmap(NULL, SECONDARY_BYTE_SIZE,
                                                PROT_READ | PROT_WRITE,
                                                MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
                                                -1, 0);
        if (s_table == MAP_FAILED) {
            perror("mmap secondary_table");
            abort();
        }
        primary_table[p_idx] = s_table;
    }

    primary_table[p_idx][s_idx] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    uintptr_t addr = (uintptr_t)slot_addr;
    assert((addr & 0x7) == 0 && "slot_addr must be 8-byte aligned");

    uintptr_t v_idx = addr >> ALIGN_SHIFT;
    uintptr_t p_idx = v_idx >> SECONDARY_BITS;
    uintptr_t s_idx = v_idx & (SECONDARY_SIZE - 1);

    if (p_idx >= PRIMARY_SIZE || primary_table[p_idx] == NULL) {
        // Return TOP grade
        go_grade_t g_top = {0};
        g_top.end = -1ULL;
        g_top.perms = 0xF;
        return g_top;
    }

    return primary_table[p_idx][s_idx];
}
