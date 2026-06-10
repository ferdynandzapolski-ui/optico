#ifndef GO_SHADOW_H
#define GO_SHADOW_H

#include "goirrt.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// GOIR shadow metadata store (hybrid tier)
// Uses a scalable two-level table architecture to map a 48-bit address space.
// Primary Table: 22 bits (Bits 47-26) -> 4M entries
// Secondary Table: 23 bits (Bits 25-3) -> 8M entries (slots for 8-byte aligned ptrs)

void __go_shadow_init(void);
void __go_shadow_store(void* slot_addr, go_grade_t g);
go_grade_t __go_shadow_load(void* slot_addr);

#ifdef __cplusplus
}
#endif

#endif // GO_SHADOW_H
