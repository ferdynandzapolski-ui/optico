#include "goirrt.h"
#include "go_trace.h"
#include <stdio.h>
#include <stdlib.h>

static go_site_t dummy_site = {"unknown", 0, "unknown"};

void __go_check_load(const void* p, go_grade_t g, size_t n) {
    fprintf(stderr, "GOIR check load: %p (size: %zu)\n", p, n);
    if ((uint64_t)p < g.base || (uint64_t)p + n > g.end) {
        __go_trace_event(GO_EVENT_CHECK_FAIL, p, g, "bounds", "OOB load", dummy_site);
    }
}

void __go_check_store(void* p, go_grade_t g, size_t n) {
    fprintf(stderr, "GOIR check store: %p (size: %zu)\n", p, n);
    if ((uint64_t)p < g.base || (uint64_t)p + n > g.end) {
        __go_trace_event(GO_EVENT_CHECK_FAIL, p, g, "bounds", "OOB store", dummy_site);
    }
}

void __go_check_free(void* p, go_grade_t g) {
    fprintf(stderr, "GOIR check free: %p\n", p);
    if ((uint64_t)p != g.base) {
        __go_trace_event(GO_EVENT_CHECK_FAIL, p, g, "bounds", "invalid free (not base)", dummy_site);
    }
}

void* __go_malloc(size_t n, go_grade_t* out_g) {
    void* p = malloc(n);
    go_grade_t g = {0};
    if (p) {
        g.base = (uint64_t)p;
        g.end = (uint64_t)p + n;
        g.flags = GO_BOUNDS_KIND_OBJECT;
        // Other fields initialized to zero/TOP for now
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

void __go_shadow_store(void* slot_addr, go_grade_t g) {
    // Stub for diagnostic tier
}

go_grade_t __go_shadow_load(void* slot_addr) {
    go_grade_t g = {0};
    return g;
}

void __go_memcpy(void* dst, go_grade_t gdst, const void* src, go_grade_t gsrc,
                 size_t n, uint32_t layout_kind) {
    fprintf(stderr, "GOIR memcpy: %p -> %p (size: %zu)\n", src, dst, n);
}
