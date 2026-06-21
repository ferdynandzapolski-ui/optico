#include "goirrt.h"
#include "go_trace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static go_site_t dummy_site = {"unknown", 0, "unknown"};

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
    }
    if (out_g) *out_g = g;
    __go_trace_event(GO_EVENT_INIT, new_p, g, "none", "realloc", dummy_site);
    return new_p;
}

// Support for LowerPass names
go_grade_t __go_grade_from_alloca(void* p, size_t n) {
    go_grade_t g = {0};
    g.base = (uint64_t)p;
    g.end = (uint64_t)p + n;
    g.flags = GO_BOUNDS_KIND_OBJECT;
    return g;
}

go_grade_t __go_grade_from_malloc(void* p, size_t n) {
    return __go_grade_from_alloca(p, n);
}

go_grade_t __go_gep_grade(go_grade_t g, int64_t offset, int64_t scale) {
    // Spatial safety: technically, object bounds don't change on GEP.
    // However, if we wanted to enforce subobject bounds, we would tighten g.base/g.end here.
    // In this MVP, we preserve object-level bounds for compatibility.
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

#include <sys/mman.h>
#include <stdatomic.h>

#define GO_PRIMARY_BITS 22
#define GO_SECONDARY_BITS 23
#define GO_SHIFT_BITS 3

#define GO_PRIMARY_SIZE (1ULL << GO_PRIMARY_BITS)
#define GO_SECONDARY_SIZE (1ULL << GO_SECONDARY_BITS)

static _Atomic(go_grade_t*) *primary_table = NULL;

static void __go_shadow_init() {
    if (atomic_load_explicit(&primary_table, memory_order_acquire) == NULL) {
        _Atomic(go_grade_t*) *new_table = mmap(NULL, GO_PRIMARY_SIZE * sizeof(_Atomic(go_grade_t*)),
                                              PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (new_table == MAP_FAILED) {
            perror("mmap primary table");
            abort();
        }
        _Atomic(go_grade_t*) *expected = NULL;
        if (!atomic_compare_exchange_strong(&primary_table, &expected, new_table)) {
            munmap(new_table, GO_PRIMARY_SIZE * sizeof(_Atomic(go_grade_t*)));
        }
    }
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    if (((uintptr_t)slot_addr & 0x7) != 0) return;
    __go_shadow_init();

    uintptr_t addr = (uintptr_t)slot_addr;
    uint32_t p_idx = (addr >> (GO_SECONDARY_BITS + GO_SHIFT_BITS)) & (GO_PRIMARY_SIZE - 1);
    uint32_t s_idx = (addr >> GO_SHIFT_BITS) & (GO_SECONDARY_SIZE - 1);

    _Atomic(go_grade_t*) *p_entry = &primary_table[p_idx];
    go_grade_t* s_table = atomic_load_explicit(p_entry, memory_order_acquire);

    if (s_table == NULL) {
        go_grade_t* new_s_table = mmap(NULL, GO_SECONDARY_SIZE * sizeof(go_grade_t),
                                      PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (new_s_table == MAP_FAILED) {
            perror("mmap secondary table");
            abort();
        }
        go_grade_t* expected = NULL;
        if (!atomic_compare_exchange_strong(p_entry, &expected, new_s_table)) {
            munmap(new_s_table, GO_SECONDARY_SIZE * sizeof(go_grade_t));
            s_table = expected;
        } else {
            s_table = new_s_table;
        }
    }

    s_table[s_idx] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
    if (((uintptr_t)slot_addr & 0x7) != 0) return g_top;

    _Atomic(go_grade_t*) *p_table = atomic_load_explicit(&primary_table, memory_order_acquire);
    if (p_table == NULL) return g_top;

    uintptr_t addr = (uintptr_t)slot_addr;
    uint32_t p_idx = (addr >> (GO_SECONDARY_BITS + GO_SHIFT_BITS)) & (GO_PRIMARY_SIZE - 1);
    uint32_t s_idx = (addr >> GO_SHIFT_BITS) & (GO_SECONDARY_SIZE - 1);

    go_grade_t* s_table = atomic_load_explicit(&p_table[p_idx], memory_order_acquire);
    if (s_table == NULL) return g_top;

    go_grade_t g = s_table[s_idx];
    if (g.end == 0) return g_top;
    return g;
}

void __go_memmove(void* dst, go_grade_t gdst, const void* src, go_grade_t gsrc,
                  size_t n, uint32_t layout_kind) {
    __go_check_store(dst, gdst, n);
    __go_check_load(src, gsrc, n);
    memmove(dst, src, n);
    if (((uintptr_t)dst & 0x7) == 0 && ((uintptr_t)src & 0x7) == 0 && (n & 0x7) == 0) {
        for (size_t i = 0; i < n; i += 8) {
            go_grade_t g = __go_shadow_load((void*)((uintptr_t)src + i));
            if (g.end != -1ULL) __go_shadow_store((void*)((uintptr_t)dst + i), g);
        }
    }
}

void __go_memcpy(void* dst, go_grade_t gdst, const void* src, go_grade_t gsrc,
                 size_t n, uint32_t layout_kind) {
    __go_memmove(dst, gdst, src, gsrc, n, layout_kind);
}

void __go_memset(void* dst, go_grade_t gdst, int v, size_t n) {
    __go_check_store(dst, gdst, n);
    memset(dst, v, n);
    if (((uintptr_t)dst & 0x7) == 0 && (n & 0x7) == 0) {
        go_grade_t g_zero = {0};
        for (size_t i = 0; i < n; i += 8) {
            __go_shadow_store((void*)((uintptr_t)dst + i), g_zero);
        }
    }
}

// Provenance
void __go_prov_expose(const void* p, go_grade_t g) {
    __go_trace_event(GO_EVENT_PROV_EXPOSE, p, g, "none", "exposed", dummy_site);
}

typedef struct {
    void* ptr;
    go_grade_t g;
} go_resolved_ptr_t;

go_resolved_ptr_t __go_inttoptr_resolve(uintptr_t i, uint32_t policy) {
    // PNVI-plain: search for a live allocation containing 'i'
    // For this MVP, return TOP if not found, or use dummy search
    go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
    go_resolved_ptr_t res = {(void*)i, g_top};
    __go_trace_event(GO_EVENT_INTTOPTR_RESOLVE, (void*)i, g_top, "none", "resolved", dummy_site);
    return res;
}
