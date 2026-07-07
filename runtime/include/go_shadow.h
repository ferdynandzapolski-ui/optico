#ifndef GO_SHADOW_H
#define GO_SHADOW_H

#include "goirrt.h"
#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

// Constants for 48-bit address space mapping
// [Primary (22 bits) | Secondary (23 bits) | Alignment (3 bits)]
#ifndef GO_PRIMARY_BITS
#define GO_PRIMARY_BITS 22
#endif

#ifndef GO_SECONDARY_BITS
#define GO_SECONDARY_BITS 23
#endif

#ifndef GO_SHIFT_BITS
#define GO_SHIFT_BITS 3
#endif

#define GO_PRIMARY_SIZE (1UL << GO_PRIMARY_BITS)
#define GO_SECONDARY_SIZE (1UL << GO_SECONDARY_BITS)

#define GO_PRIMARY_MASK (GO_PRIMARY_SIZE - 1)
#define GO_SECONDARY_MASK (GO_SECONDARY_SIZE - 1)

// Atomic secondary table pointer
typedef _Atomic(go_grade_t*) atomic_secondary_ptr_t;

/**
 * Initialize the shadow metadata store.
 * This is called lazily by store/load if not already initialized.
 */
void __go_shadow_init(void);

/**
 * Store grade metadata for a pointer-sized slot.
 * @param slot_addr Must be 8-byte aligned.
 * @param g The grade record to store.
 */
void __go_shadow_store(void* slot_addr, go_grade_t g);

/**
 * Load grade metadata for a pointer-sized slot.
 * @param slot_addr Must be 8-byte aligned.
 * @return The stored grade, or a default TOP grade if none exists.
 */
go_grade_t __go_shadow_load(void* slot_addr);

/**
 * Bulk clear shadow metadata for a range (e.g., on free or memset).
 * @param addr Start address (8-byte aligned).
 * @param size Size in bytes.
 */
void __go_shadow_clear(void* addr, size_t size);

#ifdef __cplusplus
}
#endif

#endif // GO_SHADOW_H
