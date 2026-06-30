#ifndef GO_SHADOW_H
#define GO_SHADOW_H

#include "goirrt.h"

/**
 * @brief Initialize the scalable shadow metadata store.
 *
 * This is called lazily by store/load if not already initialized.
 */
void __go_shadow_init(void);

/**
 * @brief Store a grade record for a memory slot.
 *
 * @param slot_addr The address of the memory slot (must be 8-byte aligned).
 * @param g The grade record to store.
 */
void __go_shadow_store(void* slot_addr, go_grade_t g);

/**
 * @brief Load a grade record for a memory slot.
 *
 * @param slot_addr The address of the memory slot (must be 8-byte aligned).
 * @return go_grade_t The stored grade record, or a TOP grade if not found.
 */
go_grade_t __go_shadow_load(void* slot_addr);

#endif // GO_SHADOW_H
