#ifndef GO_SHADOW_H
#define GO_SHADOW_H

#include "goirrt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the shadow metadata store.
 * Typically called by the runtime constructor.
 */
void __go_shadow_init(void);

/**
 * Store a grade record for a pointer at slot_addr.
 * slot_addr must be 8-byte aligned.
 */
void __go_shadow_store(void* slot_addr, go_grade_t g);

/**
 * Load a grade record for a pointer from slot_addr.
 * slot_addr must be 8-byte aligned.
 * Returns TOP grade if no metadata is present.
 */
go_grade_t __go_shadow_load(void* slot_addr);

#ifdef __cplusplus
}
#endif

#endif // GO_SHADOW_H
