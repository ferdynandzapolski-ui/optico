#ifndef GO_SHADOW_H
#define GO_SHADOW_H

#include "goirrt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes the shadow store. Lazily called by store/load.
 */
void __go_shadow_init(void);

/**
 * Maps a memory address to its corresponding shadow grade entry.
 * returns a pointer to the grade in the shadow table, or NULL if it could not be mapped/allocated.
 */
go_grade_t* __go_shadow_map(void* addr);

/**
 * Stores a grade record into the shadow store for the given 8-byte aligned address.
 */
void __go_shadow_store(void* slot_addr, go_grade_t g);

/**
 * Loads a grade record from the shadow store for the given 8-byte aligned address.
 * Returns a conservative TOP grade if the entry is uninitialized or the address is unaligned.
 */
go_grade_t __go_shadow_load(void* slot_addr);

#ifdef __cplusplus
}
#endif

#endif // GO_SHADOW_H
