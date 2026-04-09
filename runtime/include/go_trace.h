#ifndef GO_TRACE_H
#define GO_TRACE_H

#include "goirrt.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GO_EVENT_INIT,
    GO_EVENT_CHECK_FAIL,
    GO_EVENT_PROV_EXPOSE,
    GO_EVENT_INTTOPTR_RESOLVE,
    GO_EVENT_MEMCPY_TAINT
} go_event_type_t;

typedef struct {
    const char* file;
    int line;
    const char* fn;
} go_site_t;

void __go_trace_event(go_event_type_t type, const void* ptr, go_grade_t g, const char* component, const char* detail, go_site_t site);

#ifdef __cplusplus
}
#endif

#endif // GO_TRACE_H
