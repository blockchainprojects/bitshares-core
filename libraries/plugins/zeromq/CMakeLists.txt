#find cppzmq wrapper, installed by make of cppzmq
find_package(cppzmq)

file(GLOB HEADERS "include/graphene/zeromq/*.hpp")

add_library( graphene_zeromq
        zeromq_plugin.cpp
           )

target_link_libraries( graphene_zeromq graphene_chain graphene_app cppzmq)
target_include_directories( graphene_zeromq
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if(MSVC)
  set_source_files_properties(zeromq_plugin.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

install( TARGETS
   graphene_zeromq

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/graphene/zeromq" )

