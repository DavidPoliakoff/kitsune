set(PROG hbd)
list(APPEND CPPFLAGS -DHAVE_CONFIG_H)
list(APPEND LDFLAGS -lstdc++)
set(RUN_OPTIONS ${CMAKE_CURRENT_SOURCE_DIR}/Sort.class)
set(REQUIRES_EH_SUPPORT 1)
llvm_multisource()
