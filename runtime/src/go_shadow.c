#include "go_shadow.h"
#include <sys/mman.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef GO_PRIMARY_BITS
#define GO_PRIMARY_BITS 22
#endif

#ifndef GO_SECONDARY_BITS
#define GO_SECONDARY_BITS 23
#endif

#ifndef GO_SHIFT_BITS
#define GO_SHIFT_BITS 3
#endif

#define GO_PRIMARY_SIZE (1ULL << GO_PRIMARY_BITS)
#define GO_SECONDARY_SIZE (1ULL << GO_SECONDARY_BITS)

typedef _Atomic(go_grade_t*) atomic_secondary_ptr_t;

static _Atomic(atomic_secondary_ptr_t*) primary_table = NULL;

void __go_shadow_init(void) {
    if (atomic_load_explicit(&primary_table, memory_order_acquire) != NULL) return;

    size_t primary_bytes = GO_PRIMARY_SIZE * sizeof(atomic_secondary_ptr_t);
    // Use MAP_NORESERVE to avoid overcommitting
    atomic_secondary_ptr_t* table = mmap(NULL, primary_bytes, PROT_READ | PROT_WRITE,
                                         MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (table == MAP_FAILED) {
        perror("GOIR Runtime: failed to mmap primary shadow table");
        abort();
    }

    atomic_secondary_ptr_t* expected = NULL;
    if (!atomic_compare_exchange_strong_explicit(&primary_table, &expected, table,
                                                memory_order_acq_rel, memory_order_acquire)) {
        munmap(table, primary_bytes);
    }
}

static go_grade_t* get_secondary_table(uintptr_t primary_idx) {
    atomic_secondary_ptr_t* p_table = atomic_load_explicit(&primary_table, memory_order_acquire);
    if (!p_table) {
        __go_shadow_init();
        p_table = atomic_load_explicit(&primary_table, memory_order_acquire);
    }

    go_grade_t* s_table = atomic_load_explicit(&p_table[primary_idx], memory_order_acquire);
    if (!s_table) {
        size_t secondary_bytes = GO_SECONDARY_SIZE * sizeof(go_grade_t);
        s_table = mmap(NULL, secondary_bytes, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (s_table == MAP_FAILED) {
             perror("GOIR Runtime: failed to mmap secondary shadow table");
             abort();
        }

        go_grade_t* expected = NULL;
        if (!atomic_compare_exchange_strong_explicit(&p_table[primary_idx], &expected, s_table,
                                                    memory_order_acq_rel, memory_order_acquire)) {
            munmap(s_table, secondary_bytes);
            s_table = expected;
        }
    }
    return s_table;
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    uintptr_t addr = (uintptr_t)slot_addr;
    if (addr & 0x7) return; // 8-byte alignment required

    uintptr_t primary_idx = (addr >> (GO_SECONDARY_BITS + GO_SHIFT_BITS)) & (GO_PRIMARY_SIZE - 1);
    uintptr_t secondary_idx = (addr >> GO_SHIFT_BITS) & (GO_SECONDARY_SIZE - 1);

    go_grade_t* s_table = get_secondary_table(primary_idx);
    // Note: Non-atomic store of the 48-byte grade record.
    s_table[secondary_idx] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    uintptr_t addr = (uintptr_t)slot_addr;
    if (addr & 0x7) {
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }

    uintptr_t primary_idx = (addr >> (GO_SECONDARY_BITS + GO_SHIFT_BITS)) & (GO_PRIMARY_SIZE - 1);
    uintptr_t secondary_idx = (addr >> GO_SHIFT_BITS) & (GO_SECONDARY_SIZE - 1);

    atomic_secondary_ptr_t* p_table = atomic_load_explicit(&primary_table, memory_order_acquire);
    if (!p_table) {
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }

    go_grade_t* s_table = atomic_load_explicit(&p_table[primary_idx], memory_order_acquire);
    if (!s_table) {
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }

    // If the record is all zero, return TOP as default for uninitialized memory
    go_grade_t g = s_table[secondary_idx];
    if (g.base == 0 && g.end == 0 && g.perms == 0) {
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }

    return g;
}
