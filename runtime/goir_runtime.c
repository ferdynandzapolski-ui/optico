#include "goirrt.h"
#include "go_trace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static go_site_t dummy_site = {"unknown", 0, "unknown"};

// Global registry of all live allocations for PNVI-plain resolution
typedef struct alloc_node {
    uint64_t base;
    uint64_t end;
    go_grade_t g;
    bool exposed;
    struct alloc_node *next;
} alloc_node_t;

static alloc_node_t *live_allocs = NULL;

static void register_alloc(void* p, size_t n, go_grade_t g) {
    if (!p) return;
    alloc_node_t *node = malloc(sizeof(alloc_node_t));
    node->base = (uint64_t)p;
    node->end = (uint64_t)p + n;
    node->g = g;
    node->exposed = false;
    node->next = live_allocs;
    live_allocs = node;
}

static void unregister_alloc(void* p) {
    alloc_node_t **curr = &live_allocs;
    while (*curr) {
        if ((*curr)->base == (uint64_t)p) {
            alloc_node_t *to_free = *curr;
            *curr = to_free->next;
            free(to_free);
            return;
        }
        curr = &((*curr)->next);
    }
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
    if (!(g.perms & 0x1)) {
        go_trap("perms", "Read permission denied", p, g);
    }
}

void __go_check_store(void* p, go_grade_t g, size_t n) {
    if ((uint64_t)p < g.base || (uint64_t)p + n > g.end) {
        go_trap("bounds", "OOB store", p, g);
    }
    if (!(g.perms & 0x2)) {
        go_trap("perms", "Write permission denied", p, g);
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
        g.perms = 0x7; // RWF
        g.flags = GO_BOUNDS_KIND_OBJECT;
        register_alloc(p, n, g);
    }
    if (out_g) *out_g = g;
    __go_trace_event(GO_EVENT_INIT, p, g, "none", "malloc", dummy_site);
    return p;
}

void __go_free(void* p, go_grade_t g) {
    __go_check_free(p, g);
    unregister_alloc(p);
    free(p);
}

void* __go_realloc(void* p, size_t n, go_grade_t* out_g) {
    unregister_alloc(p);
    void* new_p = realloc(p, n);
    go_grade_t g = {0};
    if (new_p) {
        g.base = (uint64_t)new_p;
        g.end = (uint64_t)new_p + n;
        g.perms = 0x7;
        g.flags = GO_BOUNDS_KIND_OBJECT;
        register_alloc(new_p, n, g);
    }
    if (out_g) *out_g = g;
    __go_trace_event(GO_EVENT_INIT, new_p, g, "none", "realloc", dummy_site);
    return new_p;
}

go_grade_t __go_grade_from_alloca(void* p, size_t n) {
    go_grade_t g = {0};
    g.base = (uint64_t)p;
    g.end = (uint64_t)p + n;
    g.perms = 0x7;
    g.flags = GO_BOUNDS_KIND_OBJECT;
    // Note: for simplicity in MVP, we don't register stack allocs for PNVI-plain
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
    memcpy(dst, src, n);
    // TODO: Metadata propagation if layout_kind suggests pointers
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

void __go_prov_expose(const void* p, go_grade_t g, uint32_t policy) {
    if (policy == 1) { // PNVI-ae
        alloc_node_t *curr = live_allocs;
        while (curr) {
            if ((uint64_t)p >= curr->base && (uint64_t)p < curr->end) {
                curr->exposed = true;
                break;
            }
            curr = curr->next;
        }
    }
    __go_trace_event(GO_EVENT_PROV_EXPOSE, p, g, "expose", "ptrtoint", dummy_site);
}

ptr_grade_res_t __go_inttoptr_resolve(uint64_t i, uint32_t policy, uint32_t site) {
    alloc_node_t *curr = live_allocs;
    while (curr) {
        if (i >= curr->base && i < curr->end) {
            if (policy == 1 && !curr->exposed) {
                 // PNVI-ae: resolution fails if not exposed
                 break;
            }
            ptr_grade_res_t res = {(void*)i, curr->g};
            return res;
        }
        curr = curr->next;
    }

    // Fallback to TOP if no matching allocation found (or policy violation)
    go_grade_t g_top = {0}; g_top.end = -1ULL; g_top.perms = 0xF;
    ptr_grade_res_t res = {(void*)i, g_top};
    return res;
}
