#include "go_shadow.h"
#include <sys/mman.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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

#define PRIMARY_SIZE (1ULL << GO_PRIMARY_BITS)
#define SECONDARY_SIZE (1ULL << GO_SECONDARY_BITS)

typedef _Atomic(go_grade_t*) secondary_ptr_t;
static _Atomic(secondary_ptr_t*) primary_table = NULL;

void __go_shadow_init(void) {
    if (atomic_load(&primary_table) != NULL) return;

    size_t size = PRIMARY_SIZE * sizeof(secondary_ptr_t);
    void* mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (mem == MAP_FAILED) {
        perror("GOIR: failed to allocate primary shadow table");
        abort();
    }

    secondary_ptr_t* expected = NULL;
    if (!atomic_compare_exchange_strong(&primary_table, &expected, (secondary_ptr_t*)mem)) {
        munmap(mem, size);
    }
}

static go_grade_t* get_secondary_table(uintptr_t addr) {
    __go_shadow_init();
    secondary_ptr_t* pt = atomic_load(&primary_table);
    uintptr_t p_idx = (addr >> (GO_SECONDARY_BITS + GO_SHIFT_BITS)) & (PRIMARY_SIZE - 1);

    go_grade_t* st = atomic_load_explicit(&pt[p_idx], memory_order_acquire);
    if (st == NULL) {
        size_t size = SECONDARY_SIZE * sizeof(go_grade_t);
        void* mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (mem == MAP_FAILED) {
            perror("GOIR: failed to allocate secondary shadow table");
            abort();
        }

        go_grade_t* expected = NULL;
        if (atomic_compare_exchange_strong(&pt[p_idx], &expected, (go_grade_t*)mem)) {
            st = (go_grade_t*)mem;
        } else {
            munmap(mem, size);
            st = expected;
        }
    }
    return st;
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    uintptr_t addr = (uintptr_t)slot_addr;
    if (addr & ((1 << GO_SHIFT_BITS) - 1)) return; // Alignment check

    go_grade_t* st = get_secondary_table(addr);
    uintptr_t s_idx = (addr >> GO_SHIFT_BITS) & (SECONDARY_SIZE - 1);
    st[s_idx] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    uintptr_t addr = (uintptr_t)slot_addr;
    if (addr & ((1 << GO_SHIFT_BITS) - 1)) {
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }

    __go_shadow_init();
    secondary_ptr_t* pt = atomic_load(&primary_table);
    uintptr_t p_idx = (addr >> (GO_SECONDARY_BITS + GO_SHIFT_BITS)) & (PRIMARY_SIZE - 1);

    go_grade_t* st = atomic_load_explicit(&pt[p_idx], memory_order_acquire);
    if (st == NULL) {
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }

    uintptr_t s_idx = (addr >> GO_SHIFT_BITS) & (SECONDARY_SIZE - 1);
    go_grade_t g = st[s_idx];
    // Simple heuristic: if base is 0, end is 0, and perms is 0, assume uninitialized/TOP.
    // In a more robust implementation, we might use a dedicated bit in flags.
    if (g.base == 0 && g.end == 0 && g.perms == 0) {
         go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
         return g_top;
    }
    return g;
}
