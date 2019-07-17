add_executable( yoyow_node main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling yoyow_node with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

# We have to link against graphene_debug_witness because deficiency in our API infrastructure doesn't allow plugins to be fully abstracted #246
target_link_libraries( yoyow_node
                       PRIVATE graphene_app graphene_account_history graphene_market_history graphene_witness graphene_chain graphene_debug_witness graphene_non_consensus graphene_egenesis_full fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   yoyow_node

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
