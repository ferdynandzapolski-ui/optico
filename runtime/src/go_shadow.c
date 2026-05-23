#include "go_shadow.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdatomic.h>
#include <assert.h>

// Shadow table bit parameters for a 48-bit address space
// Bits 47-26: Primary index (22 bits)
// Bits 25-3:  Secondary index (23 bits)
// Bits 2-0:   Alignment (8-byte, ignored)
#define PT_BITS 22
#define ST_BITS 23
#define SHIFT 3

#define PT_SIZE (1ULL << PT_BITS)
#define ST_SIZE (1ULL << ST_BITS)

static go_grade_t** primary_table = NULL;

__attribute__((constructor))
static void __go_shadow_init() {
    size_t pt_bytes = PT_SIZE * sizeof(go_grade_t*);
    primary_table = (go_grade_t**)mmap(NULL, pt_bytes, PROT_READ | PROT_WRITE,
                                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (primary_table == MAP_FAILED) {
        perror("GOIR: Failed to mmap primary shadow table");
        abort();
    }
}

static go_grade_t* __go_get_secondary_table(uint64_t pt_idx) {
    go_grade_t* st = __atomic_load_n(&primary_table[pt_idx], __ATOMIC_ACQUIRE);
    if (st == NULL) {
        size_t st_bytes = ST_SIZE * sizeof(go_grade_t);
        go_grade_t* new_st = (go_grade_t*)mmap(NULL, st_bytes, PROT_READ | PROT_WRITE,
                                             MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (new_st == MAP_FAILED) {
            perror("GOIR: Failed to mmap secondary shadow table");
            abort();
        }

        go_grade_t* expected = NULL;
        if (!__atomic_compare_exchange_n(&primary_table[pt_idx], &expected, new_st,
                                         false, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE)) {
            // Someone else allocated it
            munmap(new_st, st_bytes);
            st = expected;
        } else {
            st = new_st;
        }
    }
    return st;
}

extern go_grade_t __go_get_top_grade();

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    uintptr_t addr = (uintptr_t)slot_addr;
    assert((addr & 0x7) == 0 && "Shadow store: slot address must be 8-byte aligned");

    uint64_t idx = addr >> SHIFT;
    uint64_t pt_idx = idx >> ST_BITS;
    uint64_t st_idx = idx & (ST_SIZE - 1);

    if (pt_idx >= PT_SIZE) {
        fprintf(stderr, "GOIR ERROR: Shadow store address out of range: %p\n", slot_addr);
        return;
    }

    go_grade_t* st = __go_get_secondary_table(pt_idx);
    st[st_idx] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    uintptr_t addr = (uintptr_t)slot_addr;
    if (addr & 0x7) return __go_get_top_grade(); // Unaligned load returns TOP

    uint64_t idx = addr >> SHIFT;
    uint64_t pt_idx = idx >> ST_BITS;
    uint64_t st_idx = idx & (ST_SIZE - 1);

    if (pt_idx >= PT_SIZE) return __go_get_top_grade();

    go_grade_t* st = __atomic_load_n(&primary_table[pt_idx], __ATOMIC_ACQUIRE);
    if (st == NULL) return __go_get_top_grade();

    go_grade_t g = st[st_idx];
    // If g is all zeros (uninitialized), treat as TOP
    if (g.end == 0 && g.base == 0 && g.perms == 0) {
        return __go_get_top_grade();
    }

    return g;
}
