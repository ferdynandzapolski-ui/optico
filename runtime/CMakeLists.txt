include_directories(include)

add_library(goirrt SHARED
  goir_runtime.c
  src/go_shadow.c
)

target_link_libraries(goirrt PRIVATE atomic)
