#ifndef GO_SHADOW_H
#define GO_SHADOW_H

#include "goirrt.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Stores the grade record 'g' for the memory slot at 'slot_addr'.
 * 'slot_addr' must be 8-byte aligned.
 */
void __go_shadow_store(void *slot_addr, go_grade_t g);

/*
 * Loads the grade record for the memory slot at 'slot_addr'.
 * Returns a TOP grade if the entry is uninitialized or 'slot_addr' is unaligned.
 */
go_grade_t __go_shadow_load(void *slot_addr);

#ifdef __cplusplus
}
#endif

#endif // GO_SHADOW_H
