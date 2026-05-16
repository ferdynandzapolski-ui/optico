#ifndef GO_SHADOW_H
#define GO_SHADOW_H

#include "goirrt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Stores a pointer grade in the shadow metadata store.
 * @param slot_addr The address in application memory holding the pointer.
 * @param g The grade record to store.
 */
void __go_shadow_store(void* slot_addr, go_grade_t g);

/**
 * Loads a pointer grade from the shadow metadata store.
 * @param slot_addr The address in application memory holding the pointer.
 * @return The associated grade record, or a TOP grade if uninitialized.
 */
go_grade_t __go_shadow_load(void* slot_addr);

#ifdef __cplusplus
}
#endif

#endif // GO_SHADOW_H
