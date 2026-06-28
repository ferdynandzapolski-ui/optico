#ifndef GO_SHADOW_H
#define GO_SHADOW_H

#include "goirrt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Stores the grade metadata for a 8-byte aligned memory slot.
 * If slot_addr is not 8-byte aligned, the request is ignored.
 */
void __go_shadow_store(void* slot_addr, go_grade_t g);

/**
 * Loads the grade metadata for a 8-byte aligned memory slot.
 * Returns a TOP grade if the slot is uninitialized or unaligned.
 */
go_grade_t __go_shadow_load(void* slot_addr);

#ifdef __cplusplus
}
#endif

#endif // GO_SHADOW_H
