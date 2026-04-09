set(LLVM_UTILS_S_PATH "${LLVM_BINARY_DIR}/bin")

llvm_canonicalize_cmake_booleans(
  LLVM_BUILD_EXAMPLES
  LLVM_ENABLE_RTTI
  LLVM_ENABLE_ZLIB
  LLVM_ENABLE_LIBXML2
  LLVM_LINK_LLVM_DYLIB
)

# Configure LLVM tests
set(MAIN_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/llvm/lit.cfg.py")
configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/llvm/lit.site.cfg.py
)

add_custom_target(check-goir
  COMMAND lit ${CMAKE_CURRENT_BINARY_DIR}/llvm
  DEPENDS GOIRPasses goirrt
  COMMENT "Running GOIR regression tests"
  USES_TERMINAL
)
