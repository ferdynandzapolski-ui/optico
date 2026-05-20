#ifndef GO_SHADOW_H
#define GO_SHADOW_H

#include "goirrt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Stores a grade for a pointer slot.
 * slot_addr must be 8-byte aligned.
 */
void __go_shadow_store(void* slot_addr, go_grade_t g);

/**
 * Loads a grade for a pointer slot.
 * slot_addr must be 8-byte aligned.
 * Returns TOP grade if no metadata is present.
 */
go_grade_t __go_shadow_load(void* slot_addr);

#ifdef __cplusplus
}
#endif

#endif // GO_SHADOW_H
