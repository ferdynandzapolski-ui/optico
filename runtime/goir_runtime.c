#include "goirrt.h"
#include "go_trace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

static go_site_t dummy_site = {"unknown", 0, "unknown"};
static int go_prov_policy = 0; // 0: plain, 1: ae

static void __attribute__((constructor)) __go_runtime_init() {
    char* policy = getenv("GOIR_PROV_POLICY");
    if (policy) go_prov_policy = atoi(policy);
}

static void go_trap(const char* cause, const char* detail, const void* p, go_grade_t g) {
    fprintf(stderr, "GOIR TRAP: %s - %s at %p\n", cause, detail, p);
    __go_trace_event(GO_EVENT_CHECK_FAIL, p, g, cause, detail, dummy_site);
    abort();
}

static void register_alloc(uint64_t base, uint64_t end);
static void unregister_alloc(uint64_t base);

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
        register_alloc(g.base, g.end);
    }
    if (out_g) *out_g = g;
    __go_trace_event(GO_EVENT_INIT, p, g, "none", "malloc", dummy_site);
    return p;
}

void __go_free(void* p, go_grade_t g) {
    __go_check_free(p, g);
    unregister_alloc(g.base);
    free(p);
}

void* __go_realloc(void* p, size_t n, go_grade_t* out_g) {
    if (p) unregister_alloc((uint64_t)p);
    void* new_p = realloc(p, n);
    go_grade_t g = {0};
    if (new_p) {
        g.base = (uint64_t)new_p;
        g.end = (uint64_t)new_p + n;
        g.flags = GO_BOUNDS_KIND_OBJECT;
        register_alloc(g.base, g.end);
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
    register_alloc(g.base, g.end);
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

// Registry for live allocations to support PNVI
#define ALLOC_REG_CAP 4096
typedef struct {
    uint64_t base;
    uint64_t end;
    bool exposed;
    bool active;
} go_alloc_reg_t;
static go_alloc_reg_t live_allocs[ALLOC_REG_CAP];

static void register_alloc(uint64_t base, uint64_t end) {
    for (int i = 0; i < ALLOC_REG_CAP; ++i) {
        if (!live_allocs[i].active) {
            live_allocs[i].base = base;
            live_allocs[i].end = end;
            live_allocs[i].exposed = false;
            live_allocs[i].active = true;
            return;
        }
    }
}

static void unregister_alloc(uint64_t base) {
    for (int i = 0; i < ALLOC_REG_CAP; ++i) {
        if (live_allocs[i].active && live_allocs[i].base == base) {
            live_allocs[i].active = false;
            return;
        }
    }
}

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

static void propagate_shadow(void* dst, const void* src, size_t n) {
    // Basic propagation: if any part of the source range has shadow metadata,
    // copy it to the destination. This is a simplification.
    // In a real implementation, we would iterate over pointer-aligned slots.
    for (size_t i = 0; i < n; i += 8) {
        go_grade_t g = __go_shadow_load((void*)((uintptr_t)src + i));
        if (g.end != -1ULL || g.base != 0) { // Not default TOP
            __go_shadow_store((void*)((uintptr_t)dst + i), g);
        }
    }
}

void __go_memcpy(void* dst, go_grade_t gdst, const void* src, go_grade_t gsrc,
                 size_t n, uint32_t layout_kind) {
    __go_check_store(dst, gdst, n);
    __go_check_load(src, gsrc, n);
    memcpy(dst, src, n);
    propagate_shadow(dst, src, n);
}

void __go_memmove(void* dst, go_grade_t gdst, const void* src, go_grade_t gsrc,
                  size_t n, uint32_t layout_kind) {
    __go_check_store(dst, gdst, n);
    __go_check_load(src, gsrc, n);
    memmove(dst, src, n);
    propagate_shadow(dst, src, n);
}

void __go_memset(void* dst, go_grade_t gdst, int v, size_t n, uint32_t layout_kind) {
    __go_check_store(dst, gdst, n);
    memset(dst, v, n);
    // Clear shadow for memset
    for (size_t i = 0; i < n; i += 8) {
        __go_shadow_store((void*)((uintptr_t)dst + i), (go_grade_t){0, -1ULL, 0, 0, 0xF, 0, 0, 0});
    }
}

void __go_prov_expose(const void* p, go_grade_t g) {
    for (int i = 0; i < ALLOC_REG_CAP; ++i) {
        if (live_allocs[i].active && (uint64_t)p >= live_allocs[i].base && (uint64_t)p < live_allocs[i].end) {
            live_allocs[i].exposed = true;
            return;
        }
    }
}

typedef struct {
    void* ptr;
    go_grade_t g;
} inttoptr_res_t;

inttoptr_res_t __go_inttoptr_resolve(uint64_t i) {
    inttoptr_res_t res = { (void*)i, {0} };
    res.g.end = -1ULL; res.g.perms = 0xF; // Default TOP

    for (int idx = 0; idx < ALLOC_REG_CAP; ++idx) {
        if (live_allocs[idx].active && i >= live_allocs[idx].base && i < live_allocs[idx].end) {
            if (go_prov_policy == 0 || (go_prov_policy == 1 && live_allocs[idx].exposed)) {
                res.g.base = live_allocs[idx].base;
                res.g.end = live_allocs[idx].end;
                res.g.perms = 0xF;
                res.g.flags = GO_BOUNDS_KIND_OBJECT;
                return res;
            }
        }
    }
    return res;
}
