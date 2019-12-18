# MIT License
#
# Copyright (c) 2018 The ViaDuck Project
# Copyright (c) 2019 Blockchain Projects B.V.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

# - Try to find MariaDBClientPP library. This matches both the "old" client library and the new C connector.
#  Once found this will define
#  MariaDBClientPP_FOUND       - System has MariaDB client library
#  MariaDBClientPP_INCLUDE_DIR - The MariaDB client library include directories
#  MariaDBClientPP_LIBRARIES   - The MariaDB client library

# includes MariaDB
find_path(MariaDBClient_INCLUDE_DIR
   NAMES mysql.h
   PATH_SUFFIXES mariadb mysql
   )

# library MariaDB
set(BAK_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_SHARED_LIBRARY_SUFFIX})
find_library(MariaDBClient_LIBRARY
   NAMES mariadb mariadbclient mysqlclient
   PATH_SUFFIXES mariadb mysql
   )
set(CMAKE_FIND_LIBRARY_SUFFIXES ${BAK_CMAKE_FIND_LIBRARY_SUFFIXES})

# includes MariaDBPP
find_path(MariaDBClientPP_INCLUDE_DIR
   NAMES account.hpp bind.hpp concurrency.hpp connection.hpp conversion.hpp conversion_help.hpp
   data.hpp date_time.hpp decimal.hpp exceptions.hpp last_error.hpp result_set.hpp save_point.hpp
   statement.hpp time.hpp time_span.hpp transaction.hpp types.hpp
   PATH_SUFFIXES mariadb++
   )

# library MariaDBPP
set(BAK_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
find_library(MariaDBClientPP_LIBRARY
   NAMES libmariadbclientpp mariadb mariadbclientpp
   )
set(CMAKE_FIND_LIBRARY_SUFFIXES ${BAK_CMAKE_FIND_LIBRARY_SUFFIXES})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args( MariaDBClientPP DEFAULT_MSG
   MariaDBClient_INCLUDE_DIR
   MariaDBClient_LIBRARY
   MariaDBClientPP_INCLUDE_DIR
   MariaDBClientPP_LIBRARY
   )

mark_as_advanced(
   MariaDBClient_INCLUDE_DIR
   MariaDBClient_LIBRARY
   MariaDBClientPP_INCLUDE_DIR
   MariaDBClientPP_LIBRARY
   )

LIST( APPEND MariaDBClientPP_LIBRARIES
   ${MariaDBClient_LIBRARY}
   ${MariaDBClientPP_LIBRARY}
   )

LIST( APPEND MariaDBClientPP_INCLUDE_DIRS
   ${MariaDBClient_INCLUDE_DIR}
   ${MariaDBClientPP_INCLUDE_DIR}
   )
