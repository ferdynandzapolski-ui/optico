#include "goirrt.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main() {
    go_grade_t g = {0x1000, 0x2000, 1, 1, 0xF, 7, 0, 0};
    void* p = (void*)0x1500;

    printf("Exposing provenance for %p...\n", p);
    __go_prov_expose(p, g);

    printf("Resolving integer 0x1500 to pointer...\n");
    ptr_grade_res_t res = __go_inttoptr_resolve(0x1500);

    printf("Resolved: ptr=%p, base=%lx, end=%lx\n", res.ptr, res.g.base, res.g.end);
    assert(res.ptr == p);
    // In MVP, resolve returns a conservative TOP-ish grade
    assert(res.g.end == -1ULL);

    printf("Provenance check passed\n");
    return 0;
}
