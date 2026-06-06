#ifndef GOIRRT_H
#define GOIRRT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GO_BOUNDS_KIND_UNKNOWN = 0,
    GO_BOUNDS_KIND_OBJECT = 1,
    GO_BOUNDS_KIND_SUBOBJECT = 2
} go_bounds_kind_t;

typedef struct {
    uint64_t base;
    uint64_t end;
    uint32_t alloc_id;
    uint32_t epoch;
    uint32_t perms;      // bitmask: R=1, W=2, F=4, X=8
    uint32_t prov_tag;
    uint64_t alias_tok;
    uint32_t flags;      // includes version, bounds-kind, exposed
} go_grade_t;

typedef struct {
    void* ptr;
    go_grade_t g;
} ptr_grade_res_t;

// Allocation wrappers
void* __go_malloc(size_t n, go_grade_t* out_g);
void  __go_free(void* p, go_grade_t g);
void* __go_realloc(void* p, size_t n, go_grade_t* out_g);

// Checks
void __go_check_load(const void* p, go_grade_t g, size_t n);
void __go_check_store(void* p, go_grade_t g, size_t n);
void __go_check_free(void* p, go_grade_t g);

// Shadow metadata (hybrid)
void __go_shadow_store(void* slot_addr, go_grade_t g);
go_grade_t __go_shadow_load(void* slot_addr);

// memcpy/memmove/memset policies
void __go_memcpy(void* dst, go_grade_t gdst, const void* src, go_grade_t gsrc,
                 size_t n, uint32_t layout_kind);
void __go_memmove(void* dst, go_grade_t gdst, const void* src, go_grade_t gsrc,
                  size_t n, uint32_t layout_kind);
void __go_memset(void* dst, go_grade_t gdst, int val, size_t n);

// Provenance
void __go_prov_expose(const void* p, go_grade_t g, uint32_t policy);
ptr_grade_res_t __go_inttoptr_resolve(uint64_t i, uint32_t policy, uint32_t site);

// LowerPass helpers
go_grade_t __go_grade_from_alloca(void* p, size_t n);
go_grade_t __go_grade_from_malloc(void* p, size_t n);
go_grade_t __go_gep_grade(go_grade_t g, int64_t offset, int64_t scale);
go_grade_t __go_join_grade(go_grade_t g1, go_grade_t g2);

#ifdef __cplusplus
}
#endif

#endif // GOIRRT_H
