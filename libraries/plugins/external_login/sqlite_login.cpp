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

#include <graphene/external_login/sqlite_login.hpp>

#include <graphene/chain/exceptions.hpp>

#include <fc/exception/exception.hpp>

#include <sstream>

using namespace SQLite;
using graphene::chain::plugin_exception;
using std::string;
using std::vector;


namespace graphene { namespace external_login {

sqlite_login::sqlite_login( const string& db_path, const string& table ) : _table( table )
{
   if( db_path.empty() )
      FC_THROW_EXCEPTION( plugin_exception, "SQLite: Please specify the path to the db." );

   try {
      _db = std::make_unique<Database>( db_path );
   }
   catch( Exception& e ) {
      FC_THROW_EXCEPTION( plugin_exception, "SQLite: Database path \"" + db_path + "\" not found." );
   }

   if( !_db->tableExists( _table ) )
      FC_THROW_EXCEPTION( plugin_exception, "SQLite: Table \"" + _table + "\" does not exist." );

   _base_query  = "SELECT password_hash_b64, password_salt_b64, allowed_apis FROM ";
   _base_query += _table;
   _base_query += " WHERE username=";
}

fc::optional<api_access_info> sqlite_login::get_api_access_info( const string& user ) const
{
   try
   {
      std::unique_ptr<Statement> query;
      query = std::make_unique<Statement>( *_db, sql_helper::make_query( _table, user ) );

      bool user_found = query->executeStep();
      if( !user_found )
         return fc::optional<api_access_info>();

      string password_hash_b64;
      string password_salt_b64;
      vector<string> allowed_apis;

      password_hash_b64 = std::move( query->getColumn(0).getString() );
      password_salt_b64 = std::move( query->getColumn(1).getString() );
      allowed_apis      = sql_helper::allowed_apis_from_string( std::move( query->getColumn(2).getString() ) );

      return api_access_info{ std::move(password_hash_b64), std::move(password_salt_b64), std::move(allowed_apis) };
   }
   catch( Exception& e )
   {
      FC_THROW_EXCEPTION( plugin_exception,
         "SQLite: " + string( e.what() )
         + " (Probably due to wrong column format, format should be: "
         + "username(VARCHAR), password_hash_b64(VARCHAR), password_salt_b64(VARCHAR), allowed_apis(VARCHAR (in a csv manner))"
         );
   }
}

}} // graphene::external_login
