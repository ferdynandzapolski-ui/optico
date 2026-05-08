#include "go_shadow.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PRIMARY_BITS 22
#define SECONDARY_BITS 23
#define ADDR_BITS 48

#define PRIMARY_SIZE (1UL << PRIMARY_BITS)
#define SECONDARY_SIZE (1UL << SECONDARY_BITS)

#define PRIMARY_MASK (PRIMARY_SIZE - 1)
#define SECONDARY_MASK (SECONDARY_SIZE - 1)

static go_grade_t **primary_table = NULL;

__attribute__((constructor))
void __go_shadow_init(void) {
    if (primary_table) return;

    size_t size = PRIMARY_SIZE * sizeof(go_grade_t *);
    primary_table = (go_grade_t **)mmap(NULL, size, PROT_READ | PROT_WRITE,
                                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (primary_table == MAP_FAILED) {
        perror("GOIR Shadow: Failed to allocate primary table");
        abort();
    }
}

static inline go_grade_t* __go_shadow_get_entry(void* slot_addr, int allocate) {
    uintptr_t addr = (uintptr_t)slot_addr;
    uintptr_t p_idx = (addr >> (SECONDARY_BITS + 3)) & PRIMARY_MASK;
    uintptr_t s_idx = (addr >> 3) & SECONDARY_MASK;

    if (!primary_table[p_idx]) {
        if (!allocate) return NULL;

        size_t size = SECONDARY_SIZE * sizeof(go_grade_t);
        go_grade_t *sec = (go_grade_t *)mmap(NULL, size, PROT_READ | PROT_WRITE,
                                             MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (sec == MAP_FAILED) {
            perror("GOIR Shadow: Failed to allocate secondary table");
            abort();
        }

        // Simple thread-unsafe check/set. For MVP this is acceptable.
        if (primary_table[p_idx]) {
            munmap(sec, size);
        } else {
            primary_table[p_idx] = sec;
        }
    }
    return &primary_table[p_idx][s_idx];
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    go_grade_t *entry = __go_shadow_get_entry(slot_addr, 1);
    if (entry) {
        *entry = g;
    }
}

go_grade_t __go_shadow_load(void* slot_addr) {
    go_grade_t *entry = __go_shadow_get_entry(slot_addr, 0);
    if (entry && entry->end != 0) {
        return *entry;
    }

    // Return TOP
    go_grade_t g_top = {0};
    g_top.end = -1ULL;
    g_top.perms = 0xF;
    return g_top;
}
