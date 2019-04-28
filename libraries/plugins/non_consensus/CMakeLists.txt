file(GLOB HEADERS "include/graphene/non_consensus/*.hpp")

add_library( graphene_non_consensus 
             non_consensus_plugin.cpp
             ${HEADERS}
           )

target_link_libraries( graphene_non_consensus graphene_chain graphene_app )
target_include_directories( graphene_non_consensus
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if(MSVC)
  set_source_files_properties( graphene_non_consensus.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

install( TARGETS
   graphene_non_consensus

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/graphene/graphene_non_consensus" )

