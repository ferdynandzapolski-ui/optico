#include "go_trace.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>

static const char* event_to_str(go_event_type_t type) {
    switch (type) {
        case GO_EVENT_INIT: return "init";
        case GO_EVENT_CHECK_FAIL: return "check_fail";
        case GO_EVENT_PROV_EXPOSE: return "prov_expose";
        case GO_EVENT_INTTOPTR_RESOLVE: return "inttoptr_resolve";
        case GO_EVENT_MEMCPY_TAINT: return "memcpy_taint";
    }
    return "unknown";
}

void __go_trace_event(go_event_type_t type, const void* ptr, go_grade_t g, const char* component, const char* detail, go_site_t site) {
    const char* trace_file_path = getenv("GOIR_TRACE_FILE");
    FILE* out = stderr;
    if (trace_file_path) {
        // NOTE: In a production tier, we would keep the file handle open or use a buffered logger.
        // For this diagnostic prototype, we append per-event to ensure traces are flushed on crash.
        out = fopen(trace_file_path, "a");
        if (!out) out = stderr;
    }

    const char* tier = getenv("GOIR_TIER");
    if (!tier) tier = "diag";

    fprintf(out, "{\"ts\": %" PRId64 ", \"tier\": \"%s\", \"event\": \"%s\", \"ptr\": \"%p\", "
                 "\"grade\": {\"base\": \"0x%" PRIx64 "\", \"end\": \"0x%" PRIx64 "\", \"alloc_id\": %" PRIu32 ", \"epoch\": %" PRIu32 ", \"perms\": %" PRIu32 ", \"prov_tag\": %" PRIu32 ", \"alias_tok\": %" PRIu64 ", \"flags\": %" PRIu32 "}, "
                 "\"fail\": {\"component\": \"%s\", \"detail\": \"%s\"}, "
                 "\"site\": {\"file\": \"%s\", \"line\": %d, \"fn\": \"%s\"}}\n",
           (int64_t)time(NULL), tier, event_to_str(type), ptr,
           g.base, g.end, g.alloc_id, g.epoch, g.perms, g.prov_tag, g.alias_tok, g.flags,
           component ? component : "none", detail ? detail : "",
           site.file ? site.file : "unknown", site.line, site.fn ? site.fn : "unknown");

    if (trace_file_path && out != stderr) {
        fclose(out);
    } else {
        fflush(out);
    }
}
