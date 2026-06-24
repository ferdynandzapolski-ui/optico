include_directories(include)

# Shadow parameters (can be overridden via -DGO_PRIMARY_BITS=...)
if(NOT DEFINED GO_PRIMARY_BITS)
    set(GO_PRIMARY_BITS 22)
endif()
if(NOT DEFINED GO_SECONDARY_BITS)
    set(GO_SECONDARY_BITS 23)
endif()
if(NOT DEFINED GO_SHIFT_BITS)
    set(GO_SHIFT_BITS 3)
endif()

add_definitions(-DGO_PRIMARY_BITS=${GO_PRIMARY_BITS})
add_definitions(-DGO_SECONDARY_BITS=${GO_SECONDARY_BITS})
add_definitions(-DGO_SHIFT_BITS=${GO_SHIFT_BITS})

add_library(goirrt SHARED
  goir_runtime.c
  src/go_shadow.c
  src/go_trace.c
)
