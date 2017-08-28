file(GLOB HEADERS "include/graphene/mobile/*.hpp")

add_library( graphene_mobile
             ${HEADERS}
             mobile_subscriptions.cpp
             mobile_plugin.cpp
             mobile_api.cpp
           )

target_link_libraries( graphene_mobile graphene_chain graphene_app graphene_db )
target_include_directories( graphene_mobile
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   graphene_mobile

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/graphene/graphene_mobile" )

