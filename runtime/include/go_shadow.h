#ifndef GO_SHADOW_H
#define GO_SHADOW_H

#include "goirrt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Stores a grade record for a pointer-sized slot in memory.
 *
 * @param slot_addr The address of the memory location holding the pointer.
 *                  Must be 8-byte aligned.
 * @param g The grade record to store.
 */
void __go_shadow_store(void* slot_addr, go_grade_t g);

/**
 * @brief Loads the grade record for a pointer-sized slot in memory.
 *
 * @param slot_addr The address of the memory location.
 *                  Must be 8-byte aligned.
 * @return go_grade_t The stored grade, or a TOP grade if no metadata exists.
 */
go_grade_t __go_shadow_load(void* slot_addr);

#ifdef __cplusplus
}
#endif

#endif // GO_SHADOW_H
