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
    uint32_t padding;    // explicit padding for 48-byte struct
} go_grade_t;

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

// memcpy policy
void __go_memcpy(void* dst, go_grade_t gdst, const void* src, go_grade_t gsrc,
                 size_t n, uint32_t layout_kind);

#include "go_shadow.h"

#ifdef __cplusplus
}
#endif

#endif // GOIRRT_H
