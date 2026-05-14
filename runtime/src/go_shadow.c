#include "go_shadow.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#define PRIMARY_BITS 22
#define SECONDARY_BITS 23
#define SHIFT 3

#define PRIMARY_SIZE (1ULL << PRIMARY_BITS)
#define SECONDARY_SIZE (1ULL << SECONDARY_BITS)

static go_grade_t **primary_table;

__attribute__((constructor))
static void __go_shadow_init() {
    size_t size = PRIMARY_SIZE * sizeof(go_grade_t *);
    primary_table = (go_grade_t **)mmap(NULL, size, PROT_READ | PROT_WRITE,
                                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (primary_table == MAP_FAILED) {
        perror("mmap primary table");
        abort();
    }
}

static go_grade_t* __go_get_secondary(uintptr_t addr) {
    uintptr_t primary_idx = (addr >> (SECONDARY_BITS + SHIFT)) & (PRIMARY_SIZE - 1);
    go_grade_t *secondary = primary_table[primary_idx];
    if (!secondary) {
        size_t size = SECONDARY_SIZE * sizeof(go_grade_t);
        void *new_sec = mmap(NULL, size, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (new_sec == MAP_FAILED) {
            perror("mmap secondary table");
            abort();
        }
        if (!__sync_bool_compare_and_swap(&primary_table[primary_idx], NULL, new_sec)) {
            munmap(new_sec, size);
            secondary = primary_table[primary_idx];
        } else {
            secondary = (go_grade_t *)new_sec;
        }
    }
    return secondary;
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    uintptr_t addr = (uintptr_t)slot_addr;
    assert((addr & 0x7) == 0 && "slot_addr must be 8-byte aligned");

    go_grade_t *secondary = __go_get_secondary(addr);
    uintptr_t secondary_idx = (addr >> SHIFT) & (SECONDARY_SIZE - 1);
    secondary[secondary_idx] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    uintptr_t addr = (uintptr_t)slot_addr;
    assert((addr & 0x7) == 0 && "slot_addr must be 8-byte aligned");

    uintptr_t primary_idx = (addr >> (SECONDARY_BITS + SHIFT)) & (PRIMARY_SIZE - 1);
    go_grade_t *secondary = primary_table[primary_idx];
    if (!secondary) {
        // Return TOP grade
        go_grade_t g_top = {0};
        g_top.end = -1ULL;
        g_top.perms = 0xF;
        return g_top;
    }
    uintptr_t secondary_idx = (addr >> SHIFT) & (SECONDARY_SIZE - 1);
    go_grade_t res = secondary[secondary_idx];
    if (res.base == 0 && res.end == 0 && res.perms == 0) {
        // Uninitialized entry in allocated secondary table
        go_grade_t g_top = {0};
        g_top.end = -1ULL;
        g_top.perms = 0xF;
        return g_top;
    }
    return res;
}
