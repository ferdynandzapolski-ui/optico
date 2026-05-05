#ifndef GO_SHADOW_H
#define GO_SHADOW_H

#include "goirrt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Stores a grade record in the shadow metadata store for a given pointer slot.
 * @param slot_addr The address of the memory location where a pointer is stored.
 * @param g The GOIR grade associated with the pointer.
 */
void __go_shadow_store(void* slot_addr, go_grade_t g);

/**
 * Loads a grade record from the shadow metadata store for a given pointer slot.
 * @param slot_addr The address of the memory location from which a pointer is loaded.
 * @return The GOIR grade associated with the pointer at that slot, or TOP if none.
 */
go_grade_t __go_shadow_load(void* slot_addr);

#ifdef __cplusplus
}
#endif

#endif // GO_SHADOW_H
