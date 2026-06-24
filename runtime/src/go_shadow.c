#include "go_shadow.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

static go_shadow_t* global_shadow = NULL;

void __go_shadow_init(void) {
    if (global_shadow) return;

    void* p = mmap(NULL, sizeof(go_shadow_t),
                   PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (p == MAP_FAILED) {
        fprintf(stderr, "GOIR: Failed to mmap primary shadow table: %s\n", strerror(errno));
        abort();
    }
    global_shadow = (go_shadow_t*)p;
}

static go_grade_t* get_secondary(uintptr_t primary_idx) {
    if (!global_shadow) __go_shadow_init();

    go_grade_t* secondary = atomic_load_explicit(&global_shadow->primary[primary_idx], memory_order_acquire);
    if (secondary == NULL) {
        size_t sec_size = GO_SECONDARY_SIZE * sizeof(go_grade_t);
        void* new_sec = mmap(NULL, sec_size, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (new_sec == MAP_FAILED) {
            fprintf(stderr, "GOIR: Failed to mmap secondary shadow table: %s\n", strerror(errno));
            abort();
        }

        go_grade_t* expected = NULL;
        if (!atomic_compare_exchange_strong(&global_shadow->primary[primary_idx], &expected, (go_grade_t*)new_sec)) {
            munmap(new_sec, sec_size);
            secondary = expected;
        } else {
            secondary = (go_grade_t*)new_sec;
        }
    }
    return secondary;
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    uintptr_t addr = (uintptr_t)slot_addr;
    if (addr & 0x7) return; // 8-byte alignment enforced

    uintptr_t idx = addr >> GO_SHIFT_BITS;
    uintptr_t primary_idx = idx >> GO_SECONDARY_BITS;
    uintptr_t secondary_idx = idx & (GO_SECONDARY_SIZE - 1);

    if (primary_idx >= GO_PRIMARY_SIZE) return;

    go_grade_t* secondary = get_secondary(primary_idx);
    secondary[secondary_idx] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    uintptr_t addr = (uintptr_t)slot_addr;
    if (addr & 0x7) {
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }

    uintptr_t idx = addr >> GO_SHIFT_BITS;
    uintptr_t primary_idx = idx >> GO_SECONDARY_BITS;
    uintptr_t secondary_idx = idx & (GO_SECONDARY_SIZE - 1);

    if (primary_idx >= GO_PRIMARY_SIZE) {
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }

    if (!global_shadow) {
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }

    go_grade_t* secondary = atomic_load_explicit(&global_shadow->primary[primary_idx], memory_order_acquire);
    if (secondary == NULL) {
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }

    go_grade_t res = secondary[secondary_idx];
    // If uninitialized (zeroed out), return TOP
    if (res.base == 0 && res.end == 0 && res.perms == 0) {
        res.end = -1ULL;
        res.perms = 0xF;
    }
    return res;
}
