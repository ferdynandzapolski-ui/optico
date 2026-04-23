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

// Improved Shadow metadata (hybrid) - Open addressing with linear probing
#define SHADOW_CAP (1 << 20)
static struct { void* addr; go_grade_t g; } shadow_map[SHADOW_CAP];

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    unsigned h = ((uintptr_t)slot_addr >> 3) & (SHADOW_CAP - 1);
    for (int i = 0; i < 16; ++i) { // Limited probing
        unsigned idx = (h + i) & (SHADOW_CAP - 1);
        if (shadow_map[idx].addr == NULL || shadow_map[idx].addr == slot_addr) {
            shadow_map[idx].addr = slot_addr;
            shadow_map[idx].g = g;
            return;
        }
    }
    // Fallback: overwrite first slot if full
    shadow_map[h].addr = slot_addr;
    shadow_map[h].g = g;
}

go_grade_t __go_shadow_load(void* slot_addr) {
    unsigned h = ((uintptr_t)slot_addr >> 3) & (SHADOW_CAP - 1);
    for (int i = 0; i < 16; ++i) {
        unsigned idx = (h + i) & (SHADOW_CAP - 1);
        if (shadow_map[idx].addr == slot_addr) return shadow_map[idx].g;
        if (shadow_map[idx].addr == NULL) break;
    }
    go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
    return g_top;
}

void __go_memcpy(void* dst, go_grade_t gdst, const void* src, go_grade_t gsrc,
                 size_t n, uint32_t layout_kind) {
    __go_check_store(dst, gdst, n);
    __go_check_load(src, gsrc, n);
    memcpy(dst, src, n);
    // In hybrid tier, if layout_kind suggests pointers, we would iterate and copy shadow metadata.
    // This is a simplified implementation.
    if (layout_kind != 0) {
        __go_trace_event(GO_EVENT_MEMCPY_TAINT, dst, gdst, "none", "memcpy with layout", dummy_site);
    }
}

void __go_memmove(void* dst, go_grade_t gdst, const void* src, go_grade_t gsrc,
                  size_t n, uint32_t layout_kind) {
    __go_check_store(dst, gdst, n);
    __go_check_load(src, gsrc, n);
    memmove(dst, src, n);
}

void __go_memset(void* dst, go_grade_t gdst, int val, size_t n) {
    __go_check_store(dst, gdst, n);
    memset(dst, val, n);
}

void __go_prov_expose(const void* p, go_grade_t g) {
    __go_trace_event(GO_EVENT_PROV_EXPOSE, p, g, "prov", "pointer exposed to integer", dummy_site);
}

itp_res_t __go_inttoptr_resolve(uint64_t i, uint32_t policy) {
    itp_res_t res;
    res.ptr = (void*)(uintptr_t)i;
    // Simplified resolution: return a TOP grade for now.
    // A full implementation would check the exposure set.
    res.g.base = 0;
    res.g.end = -1ULL;
    res.g.perms = 0xF;
    res.g.alloc_id = 0;
    res.g.epoch = 0;
    res.g.prov_tag = 0;
    res.g.alias_tok = 0;
    res.g.flags = 0;

    __go_trace_event(GO_EVENT_INTTOPTR_RESOLVE, res.ptr, res.g, "prov", "integer resolved to pointer", dummy_site);
    return res;
}
