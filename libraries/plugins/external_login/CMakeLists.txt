MESSAGE( STATUS "" )
MESSAGE( STATUS "EXTERNAL LOGIN:" )


################
##  SQLite++  ##
################

# dependencies:
# sudo apt install libsqlite3-dev
find_package( SQLiteCpp )
if( SQLiteCpp_FOUND )
   MESSAGE( STATUS "SQLITECPP_FOUND       = TRUE" )
else()
   MESSAGE( SEND_ERROR "SQLITECPP_FOUND       = FALSE" )
endif()

LIST( APPEND INCLUDES
   ${SQLITECPP_INCLUDE_DIR}
   )

LIST( APPEND SOURCES
   sqlite_login.cpp
   )

LIST( APPEND LIBRARIES
   SQLiteCpp
   sqlite3
   pthread
   dl
   )


#################
##  MariaDB++  ##
#################

list( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake" )

# dependencies:
# sudo apt install libmariadb-dev
find_package( MariaDBClientPP )
if( MariaDBClientPP_FOUND )
   MESSAGE( STATUS "MariaDBClientPP_FOUND = TRUE" )
else()
   MESSAGE( SEND_ERROR "MariaDBClientPP_FOUND = FALSE" )
endif()

LIST( APPEND INCLUDES
   ${MariaDBClientPP_INCLUDE_DIRS}
   )

LIST( APPEND SOURCES
   mariadb_login.cpp
   )

LIST( APPEND LIBRARIES
   ${MariaDBClientPP_LIBRARIES}
   mariadb
   mariadbclientpp
   )


######################
##  external_login  ##
######################

file(GLOB HEADERS "include/graphene/external_login/*.hpp")

add_library( graphene_external_login
   sql_helper.cpp
   external_login_plugin.cpp
   ${SOURCES}
   )

target_include_directories( graphene_external_login
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
   PUBLIC ${INCLUDES}
   )

target_link_libraries( graphene_external_login
   graphene_chain
   graphene_app
   ${LIBRARIES}
   )

install( TARGETS
   graphene_external_login

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
   )

INSTALL( FILES ${HEADERS} DESTINATION "include/graphene/external_login" )


