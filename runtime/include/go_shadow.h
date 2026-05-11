#ifndef GO_SHADOW_H
#define GO_SHADOW_H

#include "goirrt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes the shadow metadata store.
 * Automatically called via __attribute__((constructor)).
 */
void __go_shadow_init(void);

/**
 * Stores a grade record for a pointer-sized slot.
 * @param slot_addr The address of the memory slot holding a pointer.
 * @param g The grade to store.
 */
void __go_shadow_store(void* slot_addr, go_grade_t g);

/**
 * Loads a grade record for a pointer-sized slot.
 * @param slot_addr The address of the memory slot holding a pointer.
 * @return The stored grade, or a default TOP grade if none exists.
 */
go_grade_t __go_shadow_load(void* slot_addr);

#ifdef __cplusplus
}
#endif

#endif // GO_SHADOW_H
