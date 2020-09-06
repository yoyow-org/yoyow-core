set(DEFAULT_SYSTEM_INCLUDE_FOLDERS ${CMAKE_SOURCE_DIR}/contracts/libc++/upstream/include ${CMAKE_SOURCE_DIR}/contracts/musl/upstream/include ${Boost_INCLUDE_DIR})

set(STANDARD_INCLUDE_FOLDERS ${CMAKE_SOURCE_DIR}/contracts ${CMAKE_SOURCE_DIR}/externals/magic_get/include)

add_subdirectory(graphenelib)
add_subdirectory(musl)
add_subdirectory(libc++)


file(GLOB SKELETONS RELATIVE ${CMAKE_SOURCE_DIR}/contracts "skeleton/*")

# Documented multiple output support is broken, so properly setting up the multiple
# dependencies in the custom target is not possible.  (CMake 3.5)
add_custom_command(OUTPUT share/yy/skeleton/skeleton.cpp
                   COMMAND ${CMAKE_COMMAND} -E make_directory ../share/yy/skeleton
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_CURRENT_SOURCE_DIR}/skeleton ../share/yy/skeleton
                   DEPENDS ${SKELETONS}
                   COMMENT Copying skeleton contract...
                   VERBATIM)
add_custom_target(copy_skeleton_contract ALL DEPENDS share/yy/skeleton/skeleton.cpp)

MESSAGE( STATUS "------- cmake_install_full_includedir ------ is: "
    ${CMAKE_INSTALL_FULL_INCLUDEDIR} )

install(DIRECTORY graphenelib DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR})
install(DIRECTORY musl DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR})
install(DIRECTORY libc++ DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR})
install(DIRECTORY skeleton DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/yy)
install_directory_permissions(DIRECTORY ${CMAKE_INSTALL_FULL_DATAROOTDIR}/yy)
