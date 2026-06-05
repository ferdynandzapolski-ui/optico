include_directories(include)

add_library(goirrt SHARED
  goir_runtime.c
  src/go_mem_stub.c
  src/go_trace.c
)
