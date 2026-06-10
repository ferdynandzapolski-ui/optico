include_directories(include)

add_library(goirrt SHARED
  goir_runtime.c
  src/go_shadow.c
  src/go_trace.c
  src/go_metrics.c
  src/go_mem_stub.c
)
