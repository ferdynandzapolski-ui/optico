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

// Scalable two-level shadow metadata store (hybrid)
// 48-bit address space: Bits 47-26 (Primary), 25-3 (Secondary), 2-0 (Alignment)
#define PRIMARY_BITS 22
#define SECONDARY_BITS 23
#define PRIMARY_SIZE (1ULL << PRIMARY_BITS)
#define SECONDARY_SIZE (1ULL << SECONDARY_BITS)

static go_grade_t** primary_table = NULL;

static void __go_shadow_init() {
    if (primary_table) return;
    primary_table = (go_grade_t**)calloc(PRIMARY_SIZE, sizeof(go_grade_t*));
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    if (!primary_table) __go_shadow_init();
    uintptr_t addr = (uintptr_t)slot_addr;
    uintptr_t p_idx = (addr >> (SECONDARY_BITS + 3)) & (PRIMARY_SIZE - 1);
    uintptr_t s_idx = (addr >> 3) & (SECONDARY_SIZE - 1);

    if (!primary_table[p_idx]) {
        primary_table[p_idx] = (go_grade_t*)calloc(SECONDARY_SIZE, sizeof(go_grade_t));
    }
    primary_table[p_idx][s_idx] = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    if (!primary_table) return (go_grade_t){0, -1ULL, 0, 0, 0xF, 0, 0, 0};
    uintptr_t addr = (uintptr_t)slot_addr;
    uintptr_t p_idx = (addr >> (SECONDARY_BITS + 3)) & (PRIMARY_SIZE - 1);
    uintptr_t s_idx = (addr >> 3) & (SECONDARY_SIZE - 1);

    if (!primary_table[p_idx]) {
        go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
        return g_top;
    }
    return primary_table[p_idx][s_idx];
}

void __go_memcpy(void* dst, go_grade_t gdst, const void* src, go_grade_t gsrc,
                 size_t n, uint32_t layout_kind) {
    __go_check_store(dst, gdst, n);
    __go_check_load(src, gsrc, n);
    memcpy(dst, src, n);
}

void __go_memmove(void* dst, go_grade_t gdst, const void* src, go_grade_t gsrc,
                  size_t n, uint32_t layout_kind) {
    __go_check_store(dst, gdst, n);
    __go_check_load(src, gsrc, n);
    memmove(dst, src, n);
}

void __go_memset(void* dst, go_grade_t gdst, int v, size_t n) {
    __go_check_store(dst, gdst, n);
    memset(dst, v, n);
}

void __go_prov_expose(const void* p, go_grade_t g) {
    __go_trace_event(GO_EVENT_PROV_EXPOSE, p, g, "none", "expose", dummy_site);
}

ptr_grade_res_t __go_inttoptr_resolve(uint64_t i) {
    ptr_grade_res_t res;
    res.ptr = (void*)i;
    // Policy-dependent resolution. For MVP: simple check if address is valid heap
    // In a real implementation, we'd check the live_allocs registry.
    // For now, return a conservative grade if we can't prove it.
    go_grade_t g = {0};
    g.end = -1ULL;
    g.perms = 0xF;
    res.g = g;
    __go_trace_event(GO_EVENT_INTTOPTR_RESOLVE, res.ptr, res.g, "none", "resolve", dummy_site);
    return res;
}
