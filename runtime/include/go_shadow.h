#ifndef GO_SHADOW_H
#define GO_SHADOW_H

#include "goirrt.h"

/**
 * @file go_shadow.h
 * @brief Scalable disjoint shadow metadata store for GOIR.
 *
 * Supports a 48-bit virtual address space using a two-level table hierarchy.
 * Optimized for 8-byte aligned pointer slots.
 */

#ifndef GO_PRIMARY_BITS
#define GO_PRIMARY_BITS 22
#endif

#ifndef GO_SECONDARY_BITS
#define GO_SECONDARY_BITS 23
#endif

#ifndef GO_SHIFT_BITS
#define GO_SHIFT_BITS 3  // 8-byte alignment
#endif

/**
 * @brief Initialize the shadow metadata store.
 *
 * Typically called lazily on the first store/load if not called explicitly.
 */
void __go_shadow_init(void);

/**
 * @brief Store a grade record for a memory slot.
 *
 * @param slot_addr The 8-byte aligned address of the pointer slot.
 * @param g The grade record to store.
 */
void __go_shadow_store(void* slot_addr, go_grade_t g);

/**
 * @brief Load a grade record for a memory slot.
 *
 * @param slot_addr The 8-byte aligned address of the pointer slot.
 * @return The stored grade record, or a TOP grade if uninitialized/unaligned.
 */
go_grade_t __go_shadow_load(void* slot_addr);

#endif // GO_SHADOW_H
