#include "go_shadow.h"
#include <sys/mman.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Use atomic pointers for the primary table to ensure thread-safe lazy initialization
typedef _Atomic(go_secondary_table_t*) atomic_secondary_ptr_t;

static _Atomic(atomic_secondary_ptr_t*) primary_table = NULL;

void __go_shadow_init(void) {
    if (atomic_load_explicit(&primary_table, memory_order_acquire)) return;

    size_t primary_size = GO_PRIMARY_ENTRIES * sizeof(atomic_secondary_ptr_t);
    // Use mmap with MAP_NORESERVE for sparse allocation of the primary table
    void* mem = mmap(NULL, primary_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (mem == MAP_FAILED) {
        perror("GOIR Runtime: Failed to allocate primary shadow table");
        exit(1);
    }

    atomic_secondary_ptr_t* expected = NULL;
    if (!atomic_compare_exchange_strong_explicit(&primary_table, &expected, (atomic_secondary_ptr_t*)mem, memory_order_acq_rel, memory_order_acquire)) {
        munmap(mem, primary_size);
    }
}

static go_secondary_table_t* get_secondary_table(uintptr_t primary_idx) {
    if (!atomic_load_explicit(&primary_table, memory_order_acquire)) {
        __go_shadow_init();
    }

    atomic_secondary_ptr_t* table = atomic_load_explicit(&primary_table, memory_order_acquire);
    go_secondary_table_t* secondary = atomic_load_explicit(&table[primary_idx], memory_order_acquire);

    if (secondary) return secondary;

    size_t secondary_size = sizeof(go_secondary_table_t);
    void* mem = mmap(NULL, secondary_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (mem == MAP_FAILED) {
        perror("GOIR Runtime: Failed to allocate secondary shadow table");
        exit(1);
    }

    go_secondary_table_t* expected = NULL;
    if (atomic_compare_exchange_strong_explicit(&table[primary_idx], &expected, (go_secondary_table_t*)mem, memory_order_acq_rel, memory_order_acquire)) {
        return (go_secondary_table_t*)mem;
    } else {
        munmap(mem, secondary_size);
        return expected;
    }
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    uintptr_t addr = (uintptr_t)slot_addr;
    if (addr & ((1ULL << GO_SHIFT_BITS) - 1)) {
        // Unaligned slot - adherence to design: only 8-byte aligned slots are supported
        return;
    }

    // Mask address to 48 bits (bits 47-0)
    addr &= 0xFFFFFFFFFFFFULL;

    uintptr_t shifted_addr = addr >> GO_SHIFT_BITS;
    uintptr_t primary_idx = (shifted_addr >> GO_SECONDARY_BITS) & (GO_PRIMARY_ENTRIES - 1);
    uintptr_t secondary_idx = shifted_addr & (GO_SECONDARY_ENTRIES - 1);

    go_secondary_table_t* secondary = get_secondary_table(primary_idx);
    secondary->entries[secondary_idx] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    uintptr_t addr = (uintptr_t)slot_addr;
    go_grade_t top = {0, -1ULL, 0, 0, 0xF, 0, 0, 0};

    if (addr & ((1ULL << GO_SHIFT_BITS) - 1)) {
        // Return TOP for unaligned access
        return top;
    }

    // Mask address to 48 bits
    addr &= 0xFFFFFFFFFFFFULL;

    uintptr_t shifted_addr = addr >> GO_SHIFT_BITS;
    uintptr_t primary_idx = (shifted_addr >> GO_SECONDARY_BITS) & (GO_PRIMARY_ENTRIES - 1);
    uintptr_t secondary_idx = shifted_addr & (GO_SECONDARY_ENTRIES - 1);

    atomic_secondary_ptr_t* table = atomic_load_explicit(&primary_table, memory_order_acquire);
    if (!table) return top;

    go_secondary_table_t* secondary = atomic_load_explicit(&table[primary_idx], memory_order_acquire);
    if (!secondary) {
        return top;
    }

    return secondary->entries[secondary_idx];
}
