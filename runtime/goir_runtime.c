#include "goirrt.h"
#include <stdio.h>
#include <stdlib.h>

void __go_check_load(const void* p, go_grade_t g, size_t n) {
    fprintf(stderr, "GOIR check load: %p (size: %zu)\n", p, n);
}

void __go_check_store(void* p, go_grade_t g, size_t n) {
    fprintf(stderr, "GOIR check store: %p (size: %zu)\n", p, n);
}

void __go_check_free(void* p, go_grade_t g) {
    fprintf(stderr, "GOIR check free: %p\n", p);
}

void* __go_malloc(size_t n, go_grade_t* out_g) {
    void* p = malloc(n);
    if (out_g) {
        out_g->base = (uint64_t)p;
        out_g->end = (uint64_t)p + n;
        out_g->flags = GO_BOUNDS_KIND_OBJECT;
        // Other fields initialized to zero/TOP for now
    }
    return p;
}

void __go_free(void* p, go_grade_t g) {
    __go_check_free(p, g);
    free(p);
}

void* __go_realloc(void* p, size_t n, go_grade_t* out_g) {
    void* new_p = realloc(p, n);
    if (out_g) {
        out_g->base = (uint64_t)new_p;
        out_g->end = (uint64_t)new_p + n;
        out_g->flags = GO_BOUNDS_KIND_OBJECT;
    }
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
