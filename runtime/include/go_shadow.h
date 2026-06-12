#ifndef GO_SHADOW_H
#define GO_SHADOW_H

#include "goirrt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Stores a grade record into the shadow store for the given slot address.
 * @param slot_addr The memory address holding a pointer.
 * @param g The grade metadata associated with that pointer.
 */
void __go_shadow_store(void* slot_addr, go_grade_t g);

/**
 * @brief Loads a grade record from the shadow store for the given slot address.
 * @param slot_addr The memory address holding a pointer.
 * @return The grade metadata, or TOP if not found.
 */
go_grade_t __go_shadow_load(void* slot_addr);

/**
 * @brief Internal initialization of the shadow store.
 */
void __go_shadow_init(void);

#ifdef __cplusplus
}
#endif

#endif // GO_SHADOW_H
