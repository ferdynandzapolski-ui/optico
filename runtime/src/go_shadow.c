#include "go_shadow.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

static _Atomic(atomic_secondary_ptr_t*) primary_table = NULL;

static go_grade_t get_top_grade() {
    go_grade_t g = {0};
    g.end = -1ULL;
    g.perms = 0xF;
    return g;
}

void __go_shadow_init(void) {
    if (atomic_load_explicit(&primary_table, memory_order_acquire) != NULL) {
        return;
    }

    size_t size = GO_PRIMARY_SIZE * sizeof(atomic_secondary_ptr_t);
    atomic_secondary_ptr_t* table = (atomic_secondary_ptr_t*)mmap(
        NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    if (table == MAP_FAILED) {
        perror("GOIR Runtime: Failed to allocate primary shadow table");
        abort();
    }

    atomic_secondary_ptr_t* expected = NULL;
    if (!atomic_compare_exchange_strong_explicit(&primary_table, &expected, table,
                                                  memory_order_acq_rel, memory_order_acquire)) {
        munmap(table, size);
    }
}

static go_grade_t* get_secondary_table(uintptr_t primary_idx) {
    atomic_secondary_ptr_t* pt = atomic_load_explicit(&primary_table, memory_order_acquire);
    if (!pt) {
        __go_shadow_init();
        pt = atomic_load_explicit(&primary_table, memory_order_acquire);
    }

    go_grade_t* st = atomic_load_explicit(&pt[primary_idx], memory_order_acquire);
    if (st == NULL) {
        size_t size = GO_SECONDARY_SIZE * sizeof(go_grade_t);
        go_grade_t* new_st = (go_grade_t*)mmap(
            NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

        if (new_st == MAP_FAILED) {
            perror("GOIR Runtime: Failed to allocate secondary shadow table");
            abort();
        }

        go_grade_t* expected = NULL;
        if (!atomic_compare_exchange_strong_explicit(&pt[primary_idx], &expected, new_st,
                                                      memory_order_acq_rel, memory_order_acquire)) {
            munmap(new_st, size);
            st = atomic_load_explicit(&pt[primary_idx], memory_order_acquire);
        } else {
            st = new_st;
        }
    }
    return st;
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    uintptr_t addr = (uintptr_t)slot_addr;
    if (addr & ( (1UL << GO_SHIFT_BITS) - 1)) {
        // Unaligned store, ignore or handle? Specification says slot_addr must be aligned.
        return;
    }

    uintptr_t shifted = addr >> GO_SHIFT_BITS;
    uintptr_t primary_idx = (shifted >> GO_SECONDARY_BITS) & GO_PRIMARY_MASK;
    uintptr_t secondary_idx = shifted & GO_SECONDARY_MASK;

    go_grade_t* st = get_secondary_table(primary_idx);
    st[secondary_idx] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    uintptr_t addr = (uintptr_t)slot_addr;
    if (addr & ((1UL << GO_SHIFT_BITS) - 1)) {
        return get_top_grade();
    }

    uintptr_t shifted = addr >> GO_SHIFT_BITS;
    uintptr_t primary_idx = (shifted >> GO_SECONDARY_BITS) & GO_PRIMARY_MASK;
    uintptr_t secondary_idx = shifted & GO_SECONDARY_MASK;

    atomic_secondary_ptr_t* pt = atomic_load_explicit(&primary_table, memory_order_acquire);
    if (!pt) return get_top_grade();

    go_grade_t* st = atomic_load_explicit(&pt[primary_idx], memory_order_acquire);
    if (!st) return get_top_grade();

    go_grade_t g = st[secondary_idx];
    // If the entry is zeroed out (e.g. from mmap), we should probably return TOP
    // if that's the intended default.
    if (g.base == 0 && g.end == 0 && g.perms == 0) {
        return get_top_grade();
    }

    return g;
}

void __go_shadow_clear(void* addr, size_t size) {
    // Basic implementation: store zeroed grade for each 8-byte slot overlapping the range.
    // Optimization: could munmap or madvise(MADV_DONTNEED) entire secondary tables if range covers them.
    uintptr_t start = (uintptr_t)addr;
    uintptr_t end = start + size;

    // Align start down to the beginning of its 8-byte slot.
    // Any slot that partially overlaps with [start, end) must be cleared.
    start = start & ~7UL;

    go_grade_t zero = {0};
    for (uintptr_t p = start; p < end; p += 8) {
        __go_shadow_store((void*)p, zero);
    }
}
