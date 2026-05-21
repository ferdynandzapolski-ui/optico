#include "go_shadow.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdatomic.h>

#define PRIMARY_BITS 22
#define SECONDARY_BITS 23
#define PRIMARY_SIZE (1UL << PRIMARY_BITS)
#define SECONDARY_SIZE (1UL << SECONDARY_BITS)

static go_grade_t **primary_table = NULL;

__attribute__((constructor))
void __go_shadow_init(void) {
    if (primary_table) return;

    size_t size = PRIMARY_SIZE * sizeof(go_grade_t *);
    primary_table = (go_grade_t **)mmap(NULL, size, PROT_READ | PROT_WRITE,
                                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (primary_table == MAP_FAILED) {
        primary_table = NULL;
    }
}

static go_grade_t* get_secondary(uintptr_t primary_idx) {
    if (!primary_table) return NULL;

    go_grade_t *secondary = __atomic_load_n(&primary_table[primary_idx], __ATOMIC_ACQUIRE);
    if (secondary) return secondary;

    size_t size = SECONDARY_SIZE * sizeof(go_grade_t);
    void *new_table = mmap(NULL, size, PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    if (new_table == MAP_FAILED) return NULL;

    go_grade_t *expected = NULL;
    if (__atomic_compare_exchange_n(&primary_table[primary_idx], &expected, new_table,
                                   false, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE)) {
        return (go_grade_t *)new_table;
    } else {
        munmap(new_table, size);
        return expected;
    }
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    uintptr_t addr = (uintptr_t)slot_addr;
    assert((addr & 0x7) == 0 && "slot_addr must be 8-byte aligned");

    uintptr_t s_addr = addr >> 3;
    uintptr_t p_idx = (s_addr >> SECONDARY_BITS) & (PRIMARY_SIZE - 1);
    uintptr_t s_idx = s_addr & (SECONDARY_SIZE - 1);

    go_grade_t *secondary = get_secondary(p_idx);
    if (secondary) {
        secondary[s_idx] = g;
    }
}

go_grade_t __go_shadow_load(void* slot_addr) {
    uintptr_t addr = (uintptr_t)slot_addr;
    if ((addr & 0x7) != 0) {
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }

    uintptr_t s_addr = addr >> 3;
    uintptr_t p_idx = (s_addr >> SECONDARY_BITS) & (PRIMARY_SIZE - 1);
    uintptr_t s_idx = s_addr & (SECONDARY_SIZE - 1);

    if (!primary_table) {
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }

    go_grade_t *secondary = __atomic_load_n(&primary_table[p_idx], __ATOMIC_ACQUIRE);
    if (!secondary) {
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }

    go_grade_t res = secondary[s_idx];
    if (res.base == 0 && res.end == 0 && res.perms == 0) {
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }

    return res;
}
