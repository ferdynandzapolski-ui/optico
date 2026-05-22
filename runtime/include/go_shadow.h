#ifndef GO_SHADOW_H
#define GO_SHADOW_H

#include "goirrt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Stores a grade record into the shadow store for the given slot address.
 * slot_addr must be 8-byte aligned.
 */
void __go_shadow_store(void* slot_addr, go_grade_t g);

/**
 * Loads a grade record from the shadow store for the given slot address.
 * If no record is found or slot_addr is unaligned, returns a default TOP grade.
 */
go_grade_t __go_shadow_load(void* slot_addr);

#ifdef __cplusplus
}
#endif

#endif // GO_SHADOW_H
