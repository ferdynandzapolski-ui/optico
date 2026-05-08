#ifndef GO_SHADOW_H
#define GO_SHADOW_H

#include "goirrt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes the shadow table.
 * This is typically called automatically via __attribute__((constructor)).
 */
void __go_shadow_init(void);

/**
 * Stores the grade g into the shadow memory corresponding to slot_addr.
 * slot_addr must be 8-byte aligned.
 */
void __go_shadow_store(void* slot_addr, go_grade_t g);

/**
 * Loads the grade from the shadow memory corresponding to slot_addr.
 * If no grade was stored, returns a TOP grade.
 * slot_addr must be 8-byte aligned.
 */
go_grade_t __go_shadow_load(void* slot_addr);

#ifdef __cplusplus
}
#endif

#endif // GO_SHADOW_H
