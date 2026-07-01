#ifndef GO_SHADOW_H
#define GO_SHADOW_H

#include "goirrt.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Scalable Disjoint Shadow Metadata Store
 *
 * Maps a 48-bit address space to 48-byte grade records using a two-level table.
 * Assumes 8-byte alignment for pointer slots (3-bit shift).
 *
 * Address layout (48 bits):
 * [ Bits 47-26 (22 bits) ] [ Bits 25-3 (23 bits) ] [ Bits 2-0 (3 bits) ]
 *      Primary Index           Secondary Index            Offset
 */

#ifndef GO_PRIMARY_BITS
#define GO_PRIMARY_BITS 22
#endif

#ifndef GO_SECONDARY_BITS
#define GO_SECONDARY_BITS 23
#endif

#ifndef GO_SHIFT_BITS
#define GO_SHIFT_BITS 3
#endif

#define GO_SECONDARY_ENTRIES (1ULL << GO_SECONDARY_BITS)
#define GO_PRIMARY_ENTRIES   (1ULL << GO_PRIMARY_BITS)

// Primary table stores pointers to secondary tables
typedef struct {
    go_grade_t entries[GO_SECONDARY_ENTRIES];
} go_secondary_table_t;

// Initialize the shadow store (lazy initialization recommended)
void __go_shadow_init(void);

// Exported ABI functions (also declared in goirrt.h)
void __go_shadow_store(void* slot_addr, go_grade_t g);
go_grade_t __go_shadow_load(void* slot_addr);

#ifdef __cplusplus
}
#endif

#endif // GO_SHADOW_H
