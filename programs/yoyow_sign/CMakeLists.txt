if( UNIX AND APPLE)
  set(CMAKE_MACOSX_RPATH 0)
endif()

ADD_LIBRARY (yoyow_sign SHARED yoyow_sign.cpp)
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

TARGET_LINK_LIBRARIES( yoyow_sign
                       PRIVATE graphene_chain graphene_utilities fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

INSTALL( TARGETS
yoyow_sign

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL (FILES yoyow_sign.h DESTINATION include/yoyow_sign)
