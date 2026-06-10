#ifndef GO_METRICS_H
#define GO_METRICS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GO_METRIC_CHECK_LOAD = 0,
    GO_METRIC_CHECK_STORE,
    GO_METRIC_CHECK_FREE,
    GO_METRIC_TRAP_BOUNDS,
    GO_METRIC_TRAP_LIFE,
    GO_METRIC_TRAP_PERMS,
    GO_METRIC_TRAP_PROV,
    GO_METRIC_SHADOW_STORE,
    GO_METRIC_SHADOW_LOAD,
    GO_METRIC_ALLOC,
    GO_METRIC_MISSING_GRADE,
    GO_METRIC_MAX
} go_metric_t;

void __go_metrics_inc(go_metric_t metric);
void __go_metrics_dump(void);

#ifdef __cplusplus
}
#endif

#endif // GO_METRICS_H
