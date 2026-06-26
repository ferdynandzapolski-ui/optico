#ifndef GO_SHADOW_H
#define GO_SHADOW_H

#include "goirrt.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Scalable Two-Level Shadow Metadata Store
 * Assuming 48-bit address space.
 */

#ifndef GO_PRIMARY_BITS
#define GO_PRIMARY_BITS 22
#endif

#ifndef GO_SECONDARY_BITS
#define GO_SECONDARY_BITS 23
#endif

#ifndef GO_SHIFT_BITS
#define GO_SHIFT_BITS 3
#endif

#define GO_PRIMARY_SIZE (1UL << GO_PRIMARY_BITS)
#define GO_SECONDARY_SIZE (1UL << GO_SECONDARY_BITS)

void __go_shadow_init(void);
void __go_shadow_store(void* slot_addr, go_grade_t g);
go_grade_t __go_shadow_load(void* slot_addr);

#ifdef __cplusplus
}
#endif

#endif // GO_SHADOW_H
