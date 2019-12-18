/*
 * Copyright (c) 2019 Blockchain Projects B.V.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <graphene/external_login/mariadb_login.hpp>

#include <graphene/chain/exceptions.hpp>

#include <fc/exception/exception.hpp>

#include <mariadb++/result_set.hpp>
#include <mariadb++/exceptions.hpp>

#include <sstream>

using namespace mariadb;
using graphene::chain::plugin_exception;

namespace graphene { namespace external_login {

mariadb_login::mariadb_login( const std::string& host, const uint32_t port, const std::string& user,
   const std::string& pass, const std::string& db_name, const std::string& table, const std::string& unix_sock
   ) : _table(table)
{
   // throw if both or none is selected
   if( !( host.empty() ^ unix_sock.empty() ) )
      FC_THROW_EXCEPTION( plugin_exception, "MariaDB: Please specify either a hostname or a specific unix socket." );

   _account_setup = account::create( host, user, pass, db_name, port, unix_sock );
   _account_setup->set_connect_option( MYSQL_OPT_CONNECT_TIMEOUT, 10 );

   _connection = connection::create( _account_setup );
   try
   {
      _connection->connect();

      std::string has_table_query;
      has_table_query += "SELECT count(*) FROM information_schema.TABLES WHERE ";
      has_table_query += "(TABLE_SCHEMA = '" + db_name + "') AND ";
      has_table_query += "(TABLE_NAME = '" + table + "');";

      result_set_ref has_table_result = _connection->query( has_table_query );
      has_table_result->set_row_index(0);

      if( !has_table_result->get_signed64(0) )
         FC_THROW_EXCEPTION( plugin_exception, "MariaDB: Table \"" + _table + "\" does not exist." );
   }
   catch( exception::connection& e )
   {
      FC_THROW_EXCEPTION( plugin_exception, e.what() );
   }
}

fc::optional<api_access_info> mariadb_login::get_api_access_info( const std::string& user ) const
{
   result_set_ref result;
   try
   {
      result = _connection->query( sql_helper::make_query( _table, user ) );
   }
   catch( exception::connection& e )
   {
      FC_THROW_EXCEPTION( plugin_exception,
         "MariaDB: Probably due to wrong column format, format should be: username(VARCHAR), password_hash_b64(VARCHAR), \
         password_salt_b64(VARCHAR), allowed_apis(VARCHAR (in a csv manner))"
         );
   }

   if( !result->row_count() )
      return fc::optional<api_access_info>();

   if( !result->next() )
      FC_THROW_EXCEPTION( plugin_exception, "MariaDB: Error while fetching the user results." );

   std::string password_hash_b64( std::move( result->get_string(0) ) );
   std::string password_salt_b64( std::move( result->get_string(1) ) );
   std::vector<std::string> allowed_apis = sql_helper::allowed_apis_from_string( std::move( result->get_string(2) ) );

   return api_access_info{ std::move(password_hash_b64), std::move(password_salt_b64), std::move(allowed_apis) };
}

}} // graphene::external_login
