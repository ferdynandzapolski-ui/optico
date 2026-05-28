#ifndef GO_SHADOW_H
#define GO_SHADOW_H

#include "goirrt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Stores a grade for a pointer slot in the disjoint shadow metadata store.
 *
 * @param slot_addr The address of the memory location holding a pointer.
 * @param g The grade record to store.
 */
void __go_shadow_store(void* slot_addr, go_grade_t g);

/**
 * @brief Loads a grade for a pointer slot from the disjoint shadow metadata store.
 *
 * @param slot_addr The address of the memory location holding a pointer.
 * @return go_grade_t The grade record associated with the slot, or TOP grade if none.
 */
go_grade_t __go_shadow_load(void* slot_addr);

#ifdef __cplusplus
}
#endif

#endif // GO_SHADOW_H
