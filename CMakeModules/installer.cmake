include(InstallRequiredSystemLibraries)

if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)
endif()
