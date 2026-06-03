#ifndef GO_SHADOW_H
#define GO_SHADOW_H

#include "goirrt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Stores the grade metadata for a pointer slot.
 * @param slot_addr The address of the memory location holding a pointer.
 * @param g The grade metadata to store.
 */
void __go_shadow_store(void* slot_addr, go_grade_t g);

/**
 * Loads the grade metadata for a pointer slot.
 * @param slot_addr The address of the memory location holding a pointer.
 * @return The stored grade metadata, or a default TOP grade if none exists.
 */
go_grade_t __go_shadow_load(void* slot_addr);

#ifdef __cplusplus
}
#endif

#endif // GO_SHADOW_H
