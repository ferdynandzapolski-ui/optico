#define _GNU_SOURCE
#include "goirrt.h"
#include "go_trace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <stdatomic.h>
#include <assert.h>

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

static go_grade_t make_top_grade() {
    go_grade_t g = {0};
    g.end = -1ULL;
    g.perms = 0xF;
    return g;
}

void* __go_malloc(size_t n, go_grade_t* out_g) {
    void* p = malloc(n);
    go_grade_t g = {0};
    if (p) {
        g.base = (uint64_t)p;
        g.end = (uint64_t)p + n;
        g.flags = GO_BOUNDS_KIND_OBJECT;
        g.perms = 0x3; // R|W
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
        g.perms = 0x3; // R|W
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
    g.perms = 0x3; // R|W
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
    return make_top_grade();
}

// Two-level shadow table implementation
// 48-bit address space: 22-bit Primary, 23-bit Secondary, 3-bit offset (8-byte alignment)
#define PRIMARY_BITS 22
#define SECONDARY_BITS 23
#define SHIFT 3

#define PRIMARY_SIZE (1ULL << PRIMARY_BITS)
#define SECONDARY_SIZE (1ULL << SECONDARY_BITS)

static _Atomic(go_grade_t*) primary_table[PRIMARY_SIZE];

static go_grade_t* get_secondary(uintptr_t addr) {
    uintptr_t p_idx = (addr >> (SECONDARY_BITS + SHIFT)) & (PRIMARY_SIZE - 1);
    go_grade_t* sec = atomic_load_explicit(&primary_table[p_idx], memory_order_relaxed);
    if (!sec) {
        size_t size = SECONDARY_SIZE * sizeof(go_grade_t);
        go_grade_t* new_sec = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (new_sec == MAP_FAILED) return NULL;

        go_grade_t* expected = NULL;
        if (!atomic_compare_exchange_strong(&primary_table[p_idx], &expected, new_sec)) {
            munmap(new_sec, size);
            sec = expected;
        } else {
            sec = new_sec;
        }
    }
    return sec;
}

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    uintptr_t addr = (uintptr_t)slot_addr;
    assert((addr & 7) == 0 && "Unaligned shadow store");
    go_grade_t* sec = get_secondary(addr);
    if (sec) {
        uintptr_t s_idx = (addr >> SHIFT) & (SECONDARY_SIZE - 1);
        sec[s_idx] = g;
    }
}

go_grade_t __go_shadow_load(void* slot_addr) {
    uintptr_t addr = (uintptr_t)slot_addr;
    if (addr & 7) return make_top_grade();
    uintptr_t p_idx = (addr >> (SECONDARY_BITS + SHIFT)) & (PRIMARY_SIZE - 1);
    go_grade_t* sec = atomic_load_explicit(&primary_table[p_idx], memory_order_relaxed);
    if (!sec) return make_top_grade();

    uintptr_t s_idx = (addr >> SHIFT) & (SECONDARY_SIZE - 1);
    go_grade_t g = sec[s_idx];
    if (g.base == 0 && g.end == 0) return make_top_grade();
    return g;
}

void __go_memcpy(void* dst, go_grade_t gdst, const void* src, go_grade_t gsrc,
                 size_t n, uint32_t layout_kind) {
    __go_check_store(dst, gdst, n);
    __go_check_load(src, gsrc, n);

    // Copy payload
    memmove(dst, src, n); // Use memmove for safety

    // Hybrid: copy shadow metadata if aligned
    if (((uintptr_t)dst & 7) == 0 && ((uintptr_t)src & 7) == 0 && (n & 7) == 0) {
        for (size_t i = 0; i < n; i += 8) {
            go_grade_t gs = __go_shadow_load((void*)((uintptr_t)src + i));
            __go_shadow_store((void*)((uintptr_t)dst + i), gs);
        }
    }
}

void __go_memmove(void* dst, go_grade_t gdst, const void* src, go_grade_t gsrc,
                  size_t n, uint32_t layout_kind) {
    __go_memcpy(dst, gdst, src, gsrc, n, layout_kind);
}

void __go_memset(void* dst, go_grade_t gdst, int v, size_t n, uint32_t layout_kind) {
    __go_check_store(dst, gdst, n);
    memset(dst, v, n);

    // Clear shadow metadata
    if (((uintptr_t)dst & 7) == 0 && (n & 7) == 0) {
        go_grade_t empty = {0};
        for (size_t i = 0; i < n; i += 8) {
            __go_shadow_store((void*)((uintptr_t)dst + i), empty);
        }
    }
}

// Provenance
static _Atomic(uint32_t) exposed_ptr_count = 0;
// In a real implementation, we'd track exposed allocations.
// For this MVP, we use a simple PNVI-plain-like approach where we check if a pointer
// is within a known allocation.

void __go_prov_expose(const void* p, go_grade_t g) {
    atomic_fetch_add(&exposed_ptr_count, 1);
    // In PNVI-ae, we would mark the allocation as exposed here.
}

__go_ptr_grade_t __go_inttoptr_resolve(uint64_t i) {
    __go_ptr_grade_t res;
    res.ptr = (void*)i;
    // Minimal resolution logic: if it matches a known shadow slot, use it.
    // This is not quite right for PNVI, but good enough for MVP.
    // Real PNVI needs to scan all live allocations.
    res.grade = make_top_grade();
    return res;
}
