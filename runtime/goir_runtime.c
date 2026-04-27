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
        g.perms = 0xF; // RWF X
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
        g.perms = 0xF;
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
    g.perms = 0xF;
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

#define SHADOW_CAP (1 << 20)
static struct { void* addr; go_grade_t g; } shadow_map[SHADOW_CAP];

static void __go_shadow_clear(void* slot_addr) {
    unsigned h = ((uintptr_t)slot_addr >> 3) & (SHADOW_CAP - 1);
    for (int i = 0; i < 16; ++i) {
        unsigned idx = (h + i) & (SHADOW_CAP - 1);
        if (shadow_map[idx].addr == slot_addr) {
            shadow_map[idx].addr = NULL;
            memset(&shadow_map[idx].g, 0, sizeof(go_grade_t));
            return;
        }
        if (shadow_map[idx].addr == NULL) break;
    }
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    unsigned h = ((uintptr_t)slot_addr >> 3) & (SHADOW_CAP - 1);
    for (int i = 0; i < 16; ++i) {
        unsigned idx = (h + i) & (SHADOW_CAP - 1);
        if (shadow_map[idx].addr == NULL || shadow_map[idx].addr == slot_addr) {
            shadow_map[idx].addr = slot_addr;
            shadow_map[idx].g = g;
            return;
        }
    }
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

    for (size_t i = 0; i + sizeof(void*) <= n; i += sizeof(void*)) {
        go_grade_t g = __go_shadow_load((void*)((char*)src + i));
        if (g.end != -1ULL || g.base != 0) {
             __go_shadow_store((char*)dst + i, g);
        } else {
             __go_shadow_clear((char*)dst + i);
        }
    }
    memcpy(dst, src, n);
}

void __go_memmove(void* dst, go_grade_t gdst, const void* src, go_grade_t gsrc,
                  size_t n, uint32_t layout_kind) {
    __go_check_store(dst, gdst, n);
    __go_check_load(src, gsrc, n);

    if (dst < src || (char*)dst >= (const char*)src + n) {
        for (size_t i = 0; i + sizeof(void*) <= n; i += sizeof(void*)) {
            go_grade_t g = __go_shadow_load((void*)((char*)src + i));
            if (g.end != -1ULL || g.base != 0) {
                 __go_shadow_store((char*)dst + i, g);
            } else {
                 __go_shadow_clear((char*)dst + i);
            }
        }
    } else {
        for (size_t i = (n / sizeof(void*)) * sizeof(void*); i >= sizeof(void*); i -= sizeof(void*)) {
             size_t off = i - sizeof(void*);
             go_grade_t g = __go_shadow_load((void*)((char*)src + off));
             if (g.end != -1ULL || g.base != 0) {
                  __go_shadow_store((char*)dst + off, g);
             } else {
                  __go_shadow_clear((char*)dst + off);
             }
        }
    }
    memmove(dst, src, n);
}

void __go_memset(void* s, int c, size_t n, go_grade_t g) {
    __go_check_store(s, g, n);
    for (size_t i = 0; i + sizeof(void*) <= n; i += sizeof(void*)) {
        __go_shadow_clear((char*)s + i);
    }
    memset(s, c, n);
}

#define EXPOSURE_CAP 1024
static struct { uint64_t base; go_grade_t g; } exposure_set[EXPOSURE_CAP];
static int exposure_count = 0;

void __go_prov_expose(const void* p, go_grade_t g) {
    if (exposure_count < EXPOSURE_CAP) {
        exposure_set[exposure_count].base = g.base;
        exposure_set[exposure_count].g = g;
        exposure_count++;
    }
    __go_trace_event(GO_EVENT_PROV_EXPOSE, p, g, "prov", "exposed", dummy_site);
}

go_ptr_grade_t __go_inttoptr_resolve(uintptr_t i, int policy) {
    go_ptr_grade_t res;
    res.ptr = (void*)i;

    res.grade.base = 0;
    res.grade.end = -1ULL;
    res.grade.perms = 0xF;
    res.grade.flags = 0;

    if (policy == 1) { // PNVI-ae
        for (int j = 0; j < exposure_count; ++j) {
            if (i >= exposure_set[j].g.base && i < exposure_set[j].g.end) {
                res.grade = exposure_set[j].g;
                break;
            }
        }
    }

    __go_trace_event(GO_EVENT_INTTOPTR_RESOLVE, (void*)i, res.grade, "prov", "resolved", dummy_site);
    return res;
}
