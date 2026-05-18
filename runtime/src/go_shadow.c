#include "go_shadow.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

/*
 * 48-bit address space mapping:
 * Bits 47-26: Primary Index (22 bits) -> 4M entries
 * Bits 25-3:  Secondary Index (23 bits) -> 8M entries
 * Bits 2-0:   Alignment (3 bits)
 */

#define PRIMARY_BITS 22
#define SECONDARY_BITS 23
#define SHIFT_BITS 3

#define PRIMARY_SIZE (1UL << PRIMARY_BITS)
#define SECONDARY_SIZE (1UL << SECONDARY_BITS)

typedef go_grade_t* secondary_table_t;

static secondary_table_t* primary_table = NULL;

__attribute__((constructor))
static void __go_shadow_init() {
    size_t size = PRIMARY_SIZE * sizeof(secondary_table_t);
    primary_table = (secondary_table_t*)mmap(NULL, size, PROT_READ | PROT_WRITE,
                                             MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (primary_table == MAP_FAILED) {
        perror("GOIR shadow: failed to allocate primary table");
        abort();
    }
}

static secondary_table_t __go_get_secondary_table(uintptr_t addr, int allocate) {
    uintptr_t pi = (addr >> (SECONDARY_BITS + SHIFT_BITS)) & (PRIMARY_SIZE - 1);
    secondary_table_t sec = __atomic_load_n(&primary_table[pi], __ATOMIC_RELAXED);

    if (sec == NULL && allocate) {
        size_t size = SECONDARY_SIZE * sizeof(go_grade_t);
        secondary_table_t new_sec = (secondary_table_t)mmap(NULL, size, PROT_READ | PROT_WRITE,
                                                            MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (new_sec == MAP_FAILED) {
            perror("GOIR shadow: failed to allocate secondary table");
            abort();
        }

        secondary_table_t expected = NULL;
        if (!__atomic_compare_exchange_n(&primary_table[pi], &expected, new_sec,
                                         false, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE)) {
            munmap(new_sec, size);
            sec = expected;
        } else {
            sec = new_sec;
        }
    }
    return sec;
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    uintptr_t addr = (uintptr_t)slot_addr;
    assert((addr & 0x7) == 0 && "Shadow store address must be 8-byte aligned");

    secondary_table_t sec = __go_get_secondary_table(addr, 1);
    uintptr_t si = (addr >> SHIFT_BITS) & (SECONDARY_SIZE - 1);
    sec[si] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    uintptr_t addr = (uintptr_t)slot_addr;
    assert((addr & 0x7) == 0 && "Shadow load address must be 8-byte aligned");

    secondary_table_t sec = __go_get_secondary_table(addr, 0);
    if (sec == NULL) {
        go_grade_t top = {0};
        top.end = -1ULL;
        top.perms = 0xF;
        return top;
    }

    uintptr_t si = (addr >> SHIFT_BITS) & (SECONDARY_SIZE - 1);
    go_grade_t g = sec[si];

    // If grade is all zeros, return TOP as default
    if (g.base == 0 && g.end == 0 && g.perms == 0) {
        g.end = -1ULL;
        g.perms = 0xF;
    }

    return g;
}
