#ifndef GO_SHADOW_H
#define GO_SHADOW_H

#include "goirrt.h"

/**
 * @brief Initialize the shadow metadata store.
 *
 * Sets up the primary table for the two-level shadow store.
 */
void __go_shadow_init(void);

/**
 * @brief Store grade metadata for a pointer slot.
 *
 * @param slot_addr The address of the pointer in memory.
 * @param g The grade metadata to store.
 */
void __go_shadow_store(void* slot_addr, go_grade_t g);

/**
 * @brief Load grade metadata for a pointer slot.
 *
 * @param slot_addr The address of the pointer in memory.
 * @return The stored grade metadata, or a TOP grade if not found.
 */
go_grade_t __go_shadow_load(void* slot_addr);

#endif // GO_SHADOW_H
