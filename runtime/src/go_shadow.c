#include "go_shadow.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define ADDR_BITS 48
#define PRIMARY_BITS 22
#define SECONDARY_BITS 23
#define PAGE_SHIFT 3 // 8-byte alignment

#define PRIMARY_SIZE (1ULL << PRIMARY_BITS)
#define SECONDARY_SIZE (1ULL << SECONDARY_BITS)

// Primary table: array of pointers to secondary tables
static go_grade_t** primary_table = NULL;

static go_grade_t get_top_grade() {
    go_grade_t g = {0};
    g.base = 0;
    g.end = -1ULL;
    g.perms = 0xF; // R|W|F|X
    return g;
}

__attribute__((constructor))
void __go_shadow_init(void) {
    if (primary_table) return;

    size_t size = PRIMARY_SIZE * sizeof(go_grade_t*);
    // Use MAP_NORESERVE to avoid pre-allocating swap for the sparse table
    primary_table = (go_grade_t**)mmap(NULL, size, PROT_READ | PROT_WRITE,
                                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    if (primary_table == MAP_FAILED) {
        perror("GOIR: Failed to allocate primary shadow table");
        abort();
    }
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    uintptr_t addr = (uintptr_t)slot_addr;
    uintptr_t primary_idx = (addr >> (SECONDARY_BITS + PAGE_SHIFT)) & (PRIMARY_SIZE - 1);
    uintptr_t secondary_idx = (addr >> PAGE_SHIFT) & (SECONDARY_SIZE - 1);

    if (primary_table[primary_idx] == NULL) {
        size_t size = SECONDARY_SIZE * sizeof(go_grade_t);
        void* sec = mmap(NULL, size, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (sec == MAP_FAILED) {
            perror("GOIR: Failed to allocate secondary shadow table");
            abort();
        }

        // Atomic update of primary table entry
        if (!__sync_bool_compare_and_swap(&primary_table[primary_idx], NULL, (go_grade_t*)sec)) {
            munmap(sec, size); // Someone else beat us to it
        }
    }

    primary_table[primary_idx][secondary_idx] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    uintptr_t addr = (uintptr_t)slot_addr;
    uintptr_t primary_idx = (addr >> (SECONDARY_BITS + PAGE_SHIFT)) & (PRIMARY_SIZE - 1);
    uintptr_t secondary_idx = (addr >> PAGE_SHIFT) & (SECONDARY_SIZE - 1);

    if (primary_table == NULL || primary_table[primary_idx] == NULL) {
        return get_top_grade();
    }

    go_grade_t g = primary_table[primary_idx][secondary_idx];
    // If grade is all zeros, treat as TOP (uninitialized metadata)
    if (g.base == 0 && g.end == 0 && g.perms == 0) {
        return get_top_grade();
    }
    return g;
}
