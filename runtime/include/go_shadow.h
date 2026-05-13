#ifndef GO_SHADOW_H
#define GO_SHADOW_H

#include "goirrt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Stores the grade associated with a pointer at the given memory slot.
 * @param slot_addr The address of the memory location holding the pointer.
 * @param g The grade record for the pointer.
 */
void __go_shadow_store(void* slot_addr, go_grade_t g);

/**
 * Loads the grade associated with a pointer from the given memory slot.
 * @param slot_addr The address of the memory location holding the pointer.
 * @return The associated grade record, or TOP if not found.
 */
go_grade_t __go_shadow_load(void* slot_addr);

#ifdef __cplusplus
}
#endif

#endif // GO_SHADOW_H
