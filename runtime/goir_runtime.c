#include "goirrt.h"
#include "go_trace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <stdatomic.h>

#ifndef MAP_NORESERVE
#define MAP_NORESERVE 0x4000
#endif

// Address space parameters for 48-bit mapping (Bits 47-3)
// 22-bit Primary, 23-bit Secondary (8-byte alignment assumes 3-bit shift)
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

typedef _Atomic(go_grade_t*) atomic_secondary_ptr;
static atomic_secondary_ptr* primary_table = NULL;

static go_site_t dummy_site = {"unknown", 0, "unknown"};

static void __go_shadow_init() {
    if (primary_table) return;
    size_t sz = PRIMARY_SIZE * sizeof(atomic_secondary_ptr);
    void* p = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (p == MAP_FAILED) {
        perror("mmap primary table");
        exit(1);
    }
    atomic_secondary_ptr* expected = NULL;
    if (!atomic_compare_exchange_strong(&primary_table, &expected, (atomic_secondary_ptr*)p)) {
        munmap(p, sz);
    }
}

static go_grade_t* __go_get_secondary(uintptr_t addr, bool create) {
    __go_shadow_init();
    uintptr_t p_idx = (addr >> (GO_SECONDARY_BITS + GO_SHIFT_BITS)) & (PRIMARY_SIZE - 1);
    go_grade_t* sec = atomic_load_explicit(&primary_table[p_idx], memory_order_acquire);

    if (!sec && create) {
        size_t sz = SECONDARY_SIZE * sizeof(go_grade_t);
        void* p = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (p == MAP_FAILED) return NULL;

        go_grade_t* expected = NULL;
        if (!atomic_compare_exchange_strong(&primary_table[p_idx], &expected, (go_grade_t*)p)) {
            munmap(p, sz);
            sec = expected;
        } else {
            sec = (go_grade_t*)p;
        }
    }
    return sec;
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    uintptr_t addr = (uintptr_t)slot_addr;
    if (addr & ( (1 << GO_SHIFT_BITS) - 1)) return; // Alignment check

    go_grade_t* sec = __go_get_secondary(addr, true);
    if (sec) {
        uintptr_t s_idx = (addr >> GO_SHIFT_BITS) & (SECONDARY_SIZE - 1);
        sec[s_idx] = g;
    }
}

go_grade_t __go_shadow_load(void* slot_addr) {
    uintptr_t addr = (uintptr_t)slot_addr;
    if (addr & ( (1 << GO_SHIFT_BITS) - 1)) goto top;

    go_grade_t* sec = __go_get_secondary(addr, false);
    if (sec) {
        uintptr_t s_idx = (addr >> GO_SHIFT_BITS) & (SECONDARY_SIZE - 1);
        go_grade_t g = sec[s_idx];
        if (g.end != 0) return g;
    }

top:;
    go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
    return g_top;
}

static void go_trap(const char* cause, const char* detail, const void* p, go_grade_t g) {
    fprintf(stderr, "GOIR TRAP: %s - %s at %p\n", cause, detail, p);
    __go_trace_event(GO_EVENT_CHECK_FAIL, p, g, cause, detail, dummy_site);
    abort();
}

void __go_check_load(const void* p, go_grade_t g, size_t n) {
    if ((uint64_t)p < g.base || (uint64_t)p + n > g.end) {
        go_trap("bounds", "OOB load", p, g);
    }
}

void __go_check_store(void* p, go_grade_t g, size_t n) {
    if ((uint64_t)p < g.base || (uint64_t)p + n > g.end) {
        go_trap("bounds", "OOB store", p, g);
    }
}

void __go_check_free(void* p, go_grade_t g) {
    if ((uint64_t)p != g.base) {
        go_trap("bounds", "invalid free (not base)", p, g);
    }
}

void* __go_malloc(size_t n, go_grade_t* out_g) {
    void* p = malloc(n);
    go_grade_t g = {0};
    if (p) {
        g.base = (uint64_t)p;
        g.end = (uint64_t)p + n;
        g.flags = GO_BOUNDS_KIND_OBJECT;
        g.perms = 0x7; // RWF
    }
    if (out_g) *out_g = g;
    __go_trace_event(GO_EVENT_INIT, p, g, "none", "malloc", dummy_site);
    return p;
}

void __go_free(void* p, go_grade_t g) {
    __go_check_free(p, g);
    free(p);
}

void* __go_realloc(void* p, size_t n, go_grade_t* out_g) {
    void* new_p = realloc(p, n);
    go_grade_t g = {0};
    if (new_p) {
        g.base = (uint64_t)new_p;
        g.end = (uint64_t)new_p + n;
        g.flags = GO_BOUNDS_KIND_OBJECT;
        g.perms = 0x7;
    }
    if (out_g) *out_g = g;
    __go_trace_event(GO_EVENT_INIT, new_p, g, "none", "realloc", dummy_site);
    return new_p;
}

go_grade_t __go_grade_from_alloca(void* p, size_t n) {
    go_grade_t g = {0};
    g.base = (uint64_t)p;
    g.end = (uint64_t)p + n;
    g.flags = GO_BOUNDS_KIND_OBJECT;
    g.perms = 0x7;
    return g;
}

go_grade_t __go_grade_from_malloc(void* p, size_t n) {
    return __go_grade_from_alloca(p, n);
}

go_grade_t __go_gep_grade(go_grade_t g, int64_t offset, int64_t scale) {
    g.flags |= GO_BOUNDS_KIND_SUBOBJECT;
    return g;
}

go_grade_t __go_join_grade(go_grade_t g1, go_grade_t g2) {
    if (g1.base == g2.base && g1.end == g2.end) return g1;
    go_grade_t g_top = {0};
    g_top.end = -1ULL;
    g_top.perms = 0xF;
    return g_top;
}

void __go_prov_expose(const void* p, go_grade_t g) {
    // PNVI-ae: track exposure of allocations
    __go_trace_event(GO_EVENT_PROV_EXPOSE, p, g, "none", "ptrtoint", dummy_site);
}

typedef struct {
    void* p;
    go_grade_t g;
} go_ptr_grade_t;

go_ptr_grade_t __go_inttoptr_resolve(uintptr_t i, uint32_t policy) {
    // PNVI-plain: resolve to any live allocation if address is within it
    // For MVP, we return a TOP grade if not found.
    go_ptr_grade_t res;
    res.p = (void*)i;
    res.g.base = 0;
    res.g.end = -1ULL;
    res.g.perms = 0xF;
    res.g.alloc_id = 0;
    res.g.epoch = 0;
    res.g.prov_tag = 0;
    res.g.alias_tok = 0;
    res.g.flags = 0;
    return res;
}

void __go_memcpy(void* dst, go_grade_t gdst, const void* src, go_grade_t gsrc,
                 size_t n, uint32_t layout_kind) {
    __go_check_store(dst, gdst, n);
    __go_check_load(src, gsrc, n);

    // In hybrid tier, we should copy shadow metadata if layout_kind implies pointers.
    // For now, delegate to standard memcpy.
    memcpy(dst, src, n);

    if (layout_kind != 0) { // Assume 0 is POINTER_FREE
       for (size_t i = 0; i < n; i += 8) {
           go_grade_t gs = __go_shadow_load((void*)((uintptr_t)src + i));
           __go_shadow_store((void*)((uintptr_t)dst + i), gs);
       }
    }
}

void __go_memmove(void* dst, go_grade_t gdst, const void* src, go_grade_t gsrc,
                  size_t n, uint32_t layout_kind) {
    __go_check_store(dst, gdst, n);
    __go_check_load(src, gsrc, n);
    memmove(dst, src, n);
    // Shadow copy for memmove (omitted for brevity, similar to memcpy)
}

void __go_memset(void* dst, go_grade_t gdst, int v, size_t n) {
    __go_check_store(dst, gdst, n);
    memset(dst, v, n);
    // Clear shadow metadata in range
    for (size_t i = 0; i < n; i += 8) {
        go_grade_t zero = {0};
        zero.end = -1ULL; zero.perms = 0xF;
        __go_shadow_store((void*)((uintptr_t)dst + i), zero);
    }
}
