#include "go_metrics.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

static _Atomic uint64_t counters[GO_METRIC_MAX];

static const char* metric_names[] = {
    "check_load", "check_store", "check_free",
    "trap_bounds", "trap_life", "trap_perms", "trap_prov",
    "shadow_store", "shadow_load", "alloc", "missing_grade"
};

void __go_metrics_inc(go_metric_t metric) {
    if (metric < GO_METRIC_MAX) {
        atomic_fetch_add_explicit(&counters[metric], 1, memory_order_relaxed);
    }
}

void __go_metrics_dump(void) {
    const char* out_file = getenv("GOIR_METRICS_OUT");
    FILE* f = out_file ? fopen(out_file, "w") : stderr;
    if (!f) return;

    fprintf(f, "{\n");
    for (int i = 0; i < GO_METRIC_MAX; ++i) {
        fprintf(f, "  \"%s\": %lu%s\n", metric_names[i],
                atomic_load_explicit(&counters[i], memory_order_relaxed),
                (i == GO_METRIC_MAX - 1) ? "" : ",");
    }
    fprintf(f, "}\n");

    if (out_file) fclose(f);
}

__attribute__((destructor))
static void go_metrics_fini(void) {
    __go_metrics_dump();
}
