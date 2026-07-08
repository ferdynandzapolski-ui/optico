#include "go_shadow.h"
#include <sys/mman.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#define GO_PRIMARY_BITS 22
#define GO_SECONDARY_BITS 23
#define GO_SHIFT_BITS 3

#define GO_PRIMARY_SIZE (1ULL << GO_PRIMARY_BITS)
#define GO_SECONDARY_SIZE (1ULL << GO_SECONDARY_BITS)

#define GO_PRIMARY_MASK (GO_PRIMARY_SIZE - 1)
#define GO_SECONDARY_MASK (GO_SECONDARY_SIZE - 1)

typedef _Atomic(go_grade_t*) atomic_secondary_ptr_t;

static _Atomic(atomic_secondary_ptr_t*) primary_table = NULL;

static void __go_shadow_init() {
    if (atomic_load_explicit(&primary_table, memory_order_acquire) != NULL) {
        return;
    }

    size_t pt_size = GO_PRIMARY_SIZE * sizeof(atomic_secondary_ptr_t);
    atomic_secondary_ptr_t* new_pt = (atomic_secondary_ptr_t*)mmap(NULL, pt_size,
                                        PROT_READ | PROT_WRITE,
                                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
                                        -1, 0);

    if (new_pt == MAP_FAILED) {
        perror("GOIR: Failed to allocate primary shadow table");
        abort();
    }

    atomic_secondary_ptr_t* expected = NULL;
    if (!atomic_compare_exchange_strong_explicit(&primary_table, &expected, new_pt,
                                                memory_order_acq_rel, memory_order_acquire)) {
        munmap(new_pt, pt_size);
    }
}

static go_grade_t* get_secondary_table(uintptr_t primary_idx) {
    __go_shadow_init();

    atomic_secondary_ptr_t* pt = atomic_load_explicit(&primary_table, memory_order_acquire);
    go_grade_t* st = atomic_load_explicit(&pt[primary_idx], memory_order_acquire);

    if (st == NULL) {
        size_t st_size = GO_SECONDARY_SIZE * sizeof(go_grade_t);
        go_grade_t* new_st = (go_grade_t*)mmap(NULL, st_size,
                                          PROT_READ | PROT_WRITE,
                                          MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
                                          -1, 0);

        if (new_st == MAP_FAILED) {
            perror("GOIR: Failed to allocate secondary shadow table");
            abort();
        }

        go_grade_t* expected = NULL;
        if (!atomic_compare_exchange_strong_explicit(&pt[primary_idx], &expected, new_st,
                                                    memory_order_acq_rel, memory_order_acquire)) {
            munmap(new_st, st_size);
            st = expected;
        } else {
            st = new_st;
        }
    }

    return st;
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    uintptr_t addr = (uintptr_t)slot_addr;
    if (addr & 0x7) return; // Must be 8-byte aligned

    uintptr_t slot_idx = addr >> GO_SHIFT_BITS;
    uintptr_t p_idx = (slot_idx >> GO_SECONDARY_BITS) & GO_PRIMARY_MASK;
    uintptr_t s_idx = slot_idx & GO_SECONDARY_MASK;

    go_grade_t* st = get_secondary_table(p_idx);
    st[s_idx] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    uintptr_t addr = (uintptr_t)slot_addr;

    static go_grade_t g_top = {0, -1ULL, 0, 0, 0xF, 0, 0, 0}; // Conservative TOP

    if (addr & 0x7) return g_top;

    uintptr_t slot_idx = addr >> GO_SHIFT_BITS;
    uintptr_t p_idx = (slot_idx >> GO_SECONDARY_BITS) & GO_PRIMARY_MASK;
    uintptr_t s_idx = slot_idx & GO_SECONDARY_MASK;

    atomic_secondary_ptr_t* pt = atomic_load_explicit(&primary_table, memory_order_acquire);
    if (pt == NULL) return g_top;

    go_grade_t* st = atomic_load_explicit(&pt[p_idx], memory_order_acquire);
    if (st == NULL) return g_top;

    return st[s_idx];
}
