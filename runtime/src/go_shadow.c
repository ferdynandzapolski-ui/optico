#include "go_shadow.h"
#include <sys/mman.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef MAP_NORESERVE
#define MAP_NORESERVE 0x4000
#endif

#define GO_PRIMARY_BITS 22
#define GO_SECONDARY_BITS 23
#define GO_SHIFT_BITS 3

#define GO_PRIMARY_SIZE (1ULL << GO_PRIMARY_BITS)
#define GO_SECONDARY_SIZE (1ULL << GO_SECONDARY_BITS)

typedef _Atomic(go_grade_t*) atomic_secondary_ptr_t;

static _Atomic(atomic_secondary_ptr_t*) primary_table = NULL;

void __go_shadow_init(void) {
    if (atomic_load_explicit(&primary_table, memory_order_acquire)) return;

    size_t pt_size = GO_PRIMARY_SIZE * sizeof(atomic_secondary_ptr_t);
    void* p = mmap(NULL, pt_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    if (p == MAP_FAILED) {
        perror("GOIR: Failed to mmap primary shadow table");
        abort();
    }

    atomic_secondary_ptr_t* expected = NULL;
    if (!atomic_compare_exchange_strong_explicit(&primary_table, &expected, (atomic_secondary_ptr_t*)p,
                                                 memory_order_acq_rel, memory_order_acquire)) {
        munmap(p, pt_size);
    }
}

static go_grade_t* get_secondary_table(uintptr_t addr, int allocate) {
    atomic_secondary_ptr_t* pt = atomic_load_explicit(&primary_table, memory_order_acquire);
    if (!pt) {
        __go_shadow_init();
        pt = atomic_load_explicit(&primary_table, memory_order_acquire);
    }

    uintptr_t pi = (addr >> (GO_SECONDARY_BITS + GO_SHIFT_BITS)) & (GO_PRIMARY_SIZE - 1);

    go_grade_t* st = atomic_load_explicit(&pt[pi], memory_order_acquire);
    if (st == NULL && allocate) {
        size_t st_size = GO_SECONDARY_SIZE * sizeof(go_grade_t);
        void* p = mmap(NULL, st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

        if (p == MAP_FAILED) {
            perror("GOIR: Failed to mmap secondary shadow table");
            abort();
        }

        go_grade_t* expected = NULL;
        if (!atomic_compare_exchange_strong_explicit(&primary_table[pi], &expected, (go_grade_t*)p,
                                                     memory_order_acq_rel, memory_order_acquire)) {
            munmap(p, st_size);
            st = expected;
        } else {
            st = (go_grade_t*)p;
        }
    }
    return st;
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    uintptr_t addr = (uintptr_t)slot_addr;
    if (addr & ( (1 << GO_SHIFT_BITS) - 1)) return; // Alignment check

    go_grade_t* st = get_secondary_table(addr, 1);
    uintptr_t si = (addr >> GO_SHIFT_BITS) & (GO_SECONDARY_SIZE - 1);

    // Non-atomic store of 48-byte record
    st[si] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    uintptr_t addr = (uintptr_t)slot_addr;
    if (addr & ( (1 << GO_SHIFT_BITS) - 1)) {
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }

    go_grade_t* st = get_secondary_table(addr, 0);
    if (!st) {
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }

    uintptr_t si = (addr >> GO_SHIFT_BITS) & (GO_SECONDARY_SIZE - 1);
    return st[si];
}
