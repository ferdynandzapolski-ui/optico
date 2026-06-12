#define _GNU_SOURCE
#include "go_shadow.h"
#include <sys/mman.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

/*
 * Address Mapping (48-bit Virtual Address):
 * [PI: 22 bits] [SI: 23 bits] [Offset: 3 bits]
 * PI: 47-26
 * SI: 25-3
 */

#define PRIMARY_BITS   22
#define SECONDARY_BITS 23
#define SHIFT_BITS     3

#define PRIMARY_SIZE   (1ULL << PRIMARY_BITS)
#define SECONDARY_SIZE (1ULL << SECONDARY_BITS)

#define PRIMARY_MASK   (PRIMARY_SIZE - 1)
#define SECONDARY_MASK (SECONDARY_SIZE - 1)

static _Atomic(void**) primary_table = NULL;

void __go_shadow_init(void) {
    if (atomic_load_explicit(&primary_table, memory_order_acquire) != NULL) return;

    void** pt = (void**)mmap(NULL, PRIMARY_SIZE * sizeof(void*),
                             PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    if (pt == MAP_FAILED) {
        perror("GOIR: Failed to mmap shadow primary table");
        abort();
    }

    void** expected = NULL;
    if (!atomic_compare_exchange_strong(&primary_table, &expected, pt)) {
        munmap(pt, PRIMARY_SIZE * sizeof(void*));
    }
}

static go_grade_t* get_secondary_table(uintptr_t pi) {
    void** pt = atomic_load_explicit(&primary_table, memory_order_acquire);
    if (!pt) {
        __go_shadow_init();
        pt = atomic_load_explicit(&primary_table, memory_order_acquire);
    }

    _Atomic(void*)* entry_ptr = (_Atomic(void*)*)&pt[pi];
    void* st = atomic_load_explicit(entry_ptr, memory_order_acquire);

    if (!st) {
        void* new_st = mmap(NULL, SECONDARY_SIZE * sizeof(go_grade_t),
                            PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (new_st == MAP_FAILED) {
            perror("GOIR: Failed to mmap shadow secondary table");
            abort();
        }

        void* expected = NULL;
        if (!atomic_compare_exchange_strong(entry_ptr, &expected, new_st)) {
            munmap(new_st, SECONDARY_SIZE * sizeof(go_grade_t));
            st = expected;
        } else {
            st = new_st;
        }
    }
    return (go_grade_t*)st;
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    uintptr_t addr = (uintptr_t)slot_addr;
    // We assume 8-byte alignment for pointer slots as per design.
    // If unaligned, we ignore the store for now as it's outside the spec.
    if ((addr & 0x7) != 0) return;

    uintptr_t pi = (addr >> (SECONDARY_BITS + SHIFT_BITS)) & PRIMARY_MASK;
    uintptr_t si = (addr >> SHIFT_BITS) & SECONDARY_MASK;

    go_grade_t* st = get_secondary_table(pi);
    st[si] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    uintptr_t addr = (uintptr_t)slot_addr;
    if ((addr & 0x7) != 0) {
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }

    uintptr_t pi = (addr >> (SECONDARY_BITS + SHIFT_BITS)) & PRIMARY_MASK;
    uintptr_t si = (addr >> SHIFT_BITS) & SECONDARY_MASK;

    void** pt = atomic_load_explicit(&primary_table, memory_order_acquire);
    if (!pt) {
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }

    void* st_ptr = atomic_load_explicit((_Atomic(void*)*)&pt[pi], memory_order_acquire);
    if (!st_ptr) {
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }

    go_grade_t* st = (go_grade_t*)st_ptr;
    go_grade_t g = st[si];

    // If uninitialized (base == 0 and end == 0), return TOP.
    // Note: a valid object could have base 0 if it's NULL, but end would also be 0.
    // Actually, TOP has end = -1.
    if (g.base == 0 && g.end == 0) {
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }

    return g;
}
