#include "goirrt.h"
#include <string.h>

void __go_memmove(void* dst, go_grade_t gdst, const void* src, go_grade_t gsrc,
                  size_t n, uint32_t layout_kind) {
    __go_check_store(dst, gdst, n);
    __go_check_load(src, gsrc, n);
    memmove(dst, src, n);
}

void __go_memset(void* dst, go_grade_t gdst, int val, size_t n, uint32_t layout_kind) {
    __go_check_store(dst, gdst, n);
    memset(dst, val, n);
}
