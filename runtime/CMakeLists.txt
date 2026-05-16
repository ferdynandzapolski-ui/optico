include_directories(include)

add_library(goirrt SHARED
  goir_runtime.c
  src/go_shadow.c
  src/go_trace.c
)

add_executable(test_shadow tests/test_shadow.c src/go_shadow.c)
target_link_libraries(test_shadow pthread)
