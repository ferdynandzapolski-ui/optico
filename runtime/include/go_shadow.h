#ifndef GO_SHADOW_H
#define GO_SHADOW_H

#include "goirrt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Stores a pointer grade in the disjoint shadow metadata store.
 * @param slot_addr The address in application memory where a pointer is stored.
 *                  Must be 8-byte aligned.
 * @param g The grade associated with the pointer being stored.
 */
void __go_shadow_store(void* slot_addr, go_grade_t g);

/**
 * Loads a pointer grade from the disjoint shadow metadata store.
 * @param slot_addr The address in application memory from which a pointer is loaded.
 *                  Must be 8-byte aligned.
 * @return The grade associated with the pointer at slot_addr.
 *         Returns a default TOP grade if no metadata has been stored for this region.
 */
go_grade_t __go_shadow_load(void* slot_addr);

#ifdef __cplusplus
}
#endif

#endif // GO_SHADOW_H
