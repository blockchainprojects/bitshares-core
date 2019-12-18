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

#include <boost/test/unit_test.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/program_options.hpp>

#include <fc/crypto/base64.hpp>
#include <fc/crypto/sha256.hpp>

#include <graphene/external_login/external_login_plugin.hpp>
#include <graphene/external_login/sqlite_login.hpp>
#include <graphene/external_login/mariadb_login.hpp>
#include <mariadb++/exceptions.hpp>

#include <graphene/app/api.hpp>

#include <graphene/chain/exceptions.hpp>

#include "../common/database_fixture.hpp"


#include <vector>


using namespace graphene::chain;
using namespace graphene::chain::test;
using graphene::chain::plugin_exception;

using graphene::app::login_api;

using graphene::external_login::external_login_plugin;
using graphene::external_login::sqlite_login;
using graphene::external_login::mariadb_login;

using namespace mariadb;
using namespace SQLite;

namespace bpo = boost::program_options;

struct external_login_fixture : public database_fixture
{
   external_login_fixture()
   {
      app.register_plugin<external_login_plugin>( true );
   }

   bpo::variables_map parse_cmd( std::vector<const char*>& args ) const
   {
      bpo::options_description cli;
      bpo::options_description cfg;

      auto ext_login_plugin = app.get_plugin<external_login_plugin>("external_login_plugin");
      ext_login_plugin->plugin_set_program_options( cli, cfg );

      bpo::variables_map var_map;
      bpo::store( bpo::parse_command_line( args.size(), args.data(), cfg ), var_map );

      return var_map;
   }
};

class sql_base
{
public:
   const std::string alice_name = "alice";
   const std::string alice_pass = "password";
   const std::string alice_salt = "_some_salt";
   const std::string alice_apis = "database_api, history_api, asset_api";
   std::string alice_hash_b64;
   std::string alice_salt_b64;

   const std::string table = "bitshares_api_access_info";
   std::string create_valid_table_query;
   std::string insert_valid_entry_query;
   std::string create_invalid_table_query;
   std::string insert_invalid_entry_query;

   sql_base()
   {
      auto alice_hash = fc::sha256::hash( alice_pass + alice_salt );
      alice_hash_b64  = fc::base64_encode( alice_hash.data(), alice_hash.data_size() );
      alice_salt_b64  = fc::base64_encode( alice_salt );

      // create valid table query
      create_valid_table_query += "CREATE TABLE " + table;
      create_valid_table_query += " (username VARCHAR(30) PRIMARY KEY, password_hash_b64 VARCHAR(255), password_salt_b64 VARCHAR(255), allowed_apis VARCHAR(255) );";

      // insert valid table entry query
      insert_valid_entry_query += "INSERT INTO " + table;
      insert_valid_entry_query += " (username, password_hash_b64, password_salt_b64, allowed_apis)";
      insert_valid_entry_query += " VALUES(";
      insert_valid_entry_query += "'" + alice_name     + "',";
      insert_valid_entry_query += "'" + alice_hash_b64 + "',";
      insert_valid_entry_query += "'" + alice_salt_b64 + "',";
      insert_valid_entry_query += "'" + alice_apis     + "');";

      // create valid table query
      create_invalid_table_query += "CREATE TABLE " + table;
      create_invalid_table_query += " (username VARCHAR(30) PRIMARY KEY, password_hash_b64 VARCHAR(255) );";

      // insert invalid table entry query
      insert_invalid_entry_query += "INSERT INTO " + table;
      insert_invalid_entry_query += " (username, password_hash_b64)";
      insert_invalid_entry_query += " VALUES ('alice', 'alice_hash');";
   }
};

class sqlitedb_wrapper : public sql_base
{
public:
   const std::string path = "test.db3";

   sqlitedb_wrapper()
   {
      try{
         _db = std::make_unique<Database>( path, OPEN_READWRITE | OPEN_CREATE );
      }
      catch( Exception& e ) {
         FC_THROW_EXCEPTION( fc::exception, std::string( "SQLiteError: " ) + e.what() );
      }
   }

   ~sqlitedb_wrapper()
   {
      _delete_db_locally();
   }

   void create_valid_table() const
   {
      try {
         _db->exec( create_valid_table_query );
         _db->exec( insert_valid_entry_query );
      }
      catch( Exception& e ) {
         FC_THROW_EXCEPTION( fc::exception, std::string( "SQLiteError: " ) + e.what() );
      }
   }

   void create_invalid_table() const
   {
      try {
         _db->exec( create_invalid_table_query );
         _db->exec( insert_invalid_entry_query );
      }
      catch( Exception& e ) {
         FC_THROW_EXCEPTION( fc::exception, std::string( "SQLiteError: " ) + e.what() );
      }
   }

private:
   void _delete_db_locally() const
   {
      // TODO distinguish between windows and linux ? maybe?
      std::string command;
      command = "rm -rf " + path;
      system( command.c_str() );
   }

   std::unique_ptr<Database> _db;
};

class mariadb_wrapper : public sql_base
{
public:
   const std::string host = "127.0.0.1";
   const std::string user = "alice";
   const std::string pass = "password";
   const std::string name = "bitshares-login-tests";
   const std::string port = "3306";

   mariadb_wrapper()
   {
      _account_setup = account::create( host, user, pass, name, std::stoul(port) );
      _account_setup->set_connect_option( MYSQL_OPT_CONNECT_TIMEOUT, 10 );

      _connection = connection::create( _account_setup );
      try {
         _connection->connect();
      } catch( exception::connection& e ) {
         FC_THROW_EXCEPTION( fc::exception, e.what() );
      }
   }

   ~mariadb_wrapper()
   {
      try {
         _connection->execute( "DROP TABLE " + table + ";" );
      }
      catch( exception::connection& e ) {
         edump( ( std::string( "SHOULD NEVER HAPPEN!: " ) + e.what() ) );
      }
   }

   void create_valid_table() const
   {
      try {
         _connection->execute( create_valid_table_query );
         _connection->execute( insert_valid_entry_query );
      } catch( exception::connection& e ) {
         FC_THROW_EXCEPTION( fc::exception, e.what() );
      }
   }

   void create_invalid_table() const
   {
      try {
         _connection->execute( create_invalid_table_query );
         _connection->execute( insert_invalid_entry_query );
      } catch( fc::exception& e ) {
         FC_THROW_EXCEPTION( fc::exception, e.what() );
      }
   }

private:
   connection_ref _connection;
   account_ref    _account_setup;
};

BOOST_FIXTURE_TEST_SUITE( external_login_tests, external_login_fixture )

////////////////////////////
//  plugin general tests  //
////////////////////////////

BOOST_AUTO_TEST_CASE( fail_no_strategy_selected )
{ try {

   bpo::variables_map var_map;
   GRAPHENE_REQUIRE_THROW( app.initialize_plugins( var_map ), plugin_exception );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( fail_both_strategies_selected )
{ try {

   std::vector<const char*> args = {
      "external_login",
      "--ext-login-use-sqlite",     "true",
      "--ext-login-use-mariadb",    "true"
   };

   auto var_map = parse_cmd( args );
   GRAPHENE_REQUIRE_THROW( app.initialize_plugins( var_map ), plugin_exception );

} FC_LOG_AND_RETHROW() }


/////////////////////////////
//  sqlite specific tests  //
/////////////////////////////

BOOST_AUTO_TEST_CASE( fail_sqlite_no_db_path_set )
{ try {

   std::vector<const char*> args = {
      "external_login",
      "--ext-login-use-sqlite", "true"
   };

   auto var_map = parse_cmd( args );
   GRAPHENE_REQUIRE_THROW( app.initialize_plugins( var_map ), plugin_exception );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( fail_sqlite_db_path_not_found )
{ try {

   std::vector<const char*> args = {
      "external_login",
      "--ext-login-use-sqlite",     "true",
      "--ext-login-sqlite-db-path", "./not_existent_path/db.db3"
   };

   auto var_map = parse_cmd( args );
   GRAPHENE_REQUIRE_THROW( app.initialize_plugins( var_map ), plugin_exception );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( fail_sqlite_table_not_found )
{ try {

   sqlitedb_wrapper db;

   std::vector<const char*> args = {
      "external_login",
      "--ext-login-use-sqlite",     "true",
      "--ext-login-sqlite-db-path", db.path.c_str(),
      "--ext-login-sqlite-table",   "not_existent_table"
   };

   auto var_map = parse_cmd( args );
   GRAPHENE_REQUIRE_THROW( app.initialize_plugins( var_map ), plugin_exception );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( fail_sqlite_wrong_table_format )
{ try {

   sqlitedb_wrapper db;
   // creates a table where the last to columns are missing
   db.create_invalid_table();

   sqlite_login login( db.path, db.table );
   std::string user = "alice";

   GRAPHENE_REQUIRE_THROW( login.get_api_access_info( user ), plugin_exception );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( pass_sqlite_user_found_and_not_found )
{ try {

   sqlitedb_wrapper db;
   db.create_valid_table();

   sqlite_login login( db.path, db.table );

   std::string user;

   user = "alice";
   BOOST_CHECK( login.get_api_access_info(user).valid() );

   user = "not_existent";
   BOOST_CHECK( !login.get_api_access_info(user).valid() );

} FC_LOG_AND_RETHROW() }


//////////////////////////////
//  mariadb specific tests  //
//////////////////////////////

BOOST_AUTO_TEST_CASE( fail_mariadb_no_host_no_unix_sock_set )
{ try {

   std::vector<const char*> args = {
      "external_login",
      "--ext-login-use-mariadb",       "true"
   };

   auto var_map = parse_cmd( args );
   GRAPHENE_REQUIRE_THROW( app.initialize_plugins( var_map ), plugin_exception );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( fail_mariadb_host_and_unix_sock_set )
{ try {

   std::vector<const char*> args = {
      "external_login",
      "--ext-login-use-mariadb",       "true",
      "--ext-login-mariadb-host",      "some_host",
      "--ext-login-mariadb-unix-sock", "some_unix_sock"
   };

   auto var_map = parse_cmd( args );
   GRAPHENE_REQUIRE_THROW( app.initialize_plugins( var_map ), plugin_exception );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( fail_mariadb_connection_error )
{ try {

   std::vector<const char*> args = {
      "external_login",
      "--ext-login-use-mariadb",       "true",
      "--ext-login-mariadb-host",      "some_unreachable_host",
   };

   auto var_map = parse_cmd( args );
   GRAPHENE_REQUIRE_THROW( app.initialize_plugins( var_map ), plugin_exception );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( fail_mariadb_table_not_found )
{ try {

   mariadb_wrapper db;
   db.create_invalid_table();

   std::vector<const char*> args = {
      "external_login",
      "--ext-login-use-mariadb",       "true",
      "--ext-login-mariadb-host",      db.host.c_str(),
      "--ext-login-mariadb-port",      db.port.c_str(),
      "--ext-login-mariadb-user",      db.user.c_str(),
      "--ext-login-mariadb-pass",      db.pass.c_str(),
      "--ext-login-mariadb-db-name",   db.name.c_str(),
      "--ext-login-mariadb-table",     "some_wrong_table"
   };

   auto var_map = parse_cmd( args );
   GRAPHENE_REQUIRE_THROW( app.initialize_plugins( var_map ), plugin_exception );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( fail_mariadb_wrong_table_format )
{ try {

   mariadb_wrapper db;
   db.create_invalid_table();

   mariadb_login login( db.host, std::stoul(db.port), db.user, db.pass, db.name, db.table, "" );
   std::string user = "alice";

   GRAPHENE_REQUIRE_THROW( login.get_api_access_info( user ), plugin_exception );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( pass_mariadb_user_found_and_not_found )
{ try {

   mariadb_wrapper db;
   db.create_valid_table();

   mariadb_login login( db.host, std::stoul(db.port), db.user, db.pass, db.name, db.table, "" );

   std::string user;

   user = "alice";
   BOOST_CHECK( login.get_api_access_info(user).valid() );

   user = "not_existent";
   BOOST_CHECK( !login.get_api_access_info(user).valid() );

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()