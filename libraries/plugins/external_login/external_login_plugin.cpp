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

#include <graphene/external_login/external_login_plugin.hpp>
#include <graphene/external_login/sqlite_login.hpp>
#include <graphene/external_login/mariadb_login.hpp>

#include <graphene/app/api.hpp>

using graphene::chain::plugin_exception;
using graphene::app::login_api;
using fc::optional;

using std::string;
namespace bpo = boost::program_options;

namespace graphene { namespace external_login {

namespace detail
{

class external_login_plugin_impl
{
public:
   external_login_plugin_impl( external_login_plugin& _plugin )
      : _self( _plugin )
   {
   }
   virtual ~external_login_plugin_impl()
   {
   }

   std::unique_ptr<login_strategy> login_strat;

   bool     use_sqlite        = false;
   string   sqlite_db_path    = "";
   string   sqlite_table      = "bitshares_api_access_info";

   bool     use_mariadb       = false;
   string   mariadb_host      = "";
   uint32_t mariadb_port      = 3306;
   string   mariadb_unix_sock = "";
   string   mariadb_user      = "";
   string   mariadb_pass      = "";
   string   mariadb_db_name   = "";
   string   mariadb_table     = "bitshares_api_access_info";

   void on_login( const string& user, const string& password, login_api& login_api ) const;

private:
   external_login_plugin& _self;
};

void external_login_plugin_impl::on_login( const string& user, const string& password, login_api& login_api
   ) const
{
   auto ext_login_con = login_api.api_access_info_external.connect( [this]( const string& user,
      optional<api_access_info>& api_info ) {
         api_info = this->login_strat->get_api_access_info( user );
      }
   );
   login_api.login( user, password );

   login_api.api_access_info_external.disconnect( ext_login_con );
}

} // end namespace detail

external_login_plugin::external_login_plugin()
   : my( new detail::external_login_plugin_impl( *this ) )
{
}

external_login_plugin::~external_login_plugin()
{
}

string external_login_plugin::plugin_name() const
{
   return "external_login_plugin";
}

string external_login_plugin::plugin_description() const
{
   return "Enables the api login through an external database.";
}

void external_login_plugin::plugin_set_program_options(
   bpo::options_description& cli,
   bpo::options_description& cfg
   )
{
   cli.add_options()
      ( "ext-login-use-sqlite", bpo::value<bool>(),
         "Use sqlite as the external login strategy (false)" )
      ( "ext-login-sqlite-db-path", bpo::value<string>(),
         "Path to the sqlite db (\"\")" )
      ( "ext-login-sqlite-table", bpo::value<string>(),
         "Name of the sqlite table (\"bitshares_api_access_info\")" )

      ( "ext-login-use-mariadb", bpo::value<bool>(),
         "Use mariadb as the external login strategy (false)" )
      ( "ext-login-mariadb-host", bpo::value<string>(),
         "Host address for mariadb, \"localhost\" can be specified here (\"\")" )
      ( "ext-login-mariadb-port", bpo::value<uint32_t>(),
         "Port of the mariadb host (3306)" )
      ( "ext-login-mariadb-user", bpo::value<string>(),
         "Username for mariadb database (\"\")" )
      ( "ext-login-mariadb-pass", bpo::value<string>(),
         "Password for the mariadb user (\"\")" )
      ( "ext-login-mariadb-db-name", bpo::value<string>(),
         "Name of the mariadb database (\"\")" )
      ( "ext-login-mariadb-table", bpo::value<string>(),
         "Name of the mariadb table (\"bitshares_api_access_info\")" )
      ( "ext-login-mariadb-unix-sock", bpo::value<string>(),
         "Name of the unix sock for mariadb (\"\")" )
      ;
   cfg.add(cli);
}

void external_login_plugin::plugin_initialize( const bpo::variables_map& options )
{
   if( options.count("ext-login-use-sqlite") )
      my->use_sqlite = options["ext-login-use-sqlite"].as<bool>();
   if( options.count("ext-login-use-mariadb") )
      my->use_mariadb = options["ext-login-use-mariadb"].as<bool>();

   if( !( my->use_sqlite ^ my->use_mariadb ) ) {
      FC_THROW_EXCEPTION( plugin_exception,
         "Please select either SQLite or MariaDB as an external login strategy." );
   }

   if( my->use_sqlite )
   {
      if( options.count("ext-login-sqlite-db-path") )
         my->sqlite_db_path = options["ext-login-sqlite-db-path"].as<string>();
      if( options.count("ext-login-sqlite-table") )
         my->sqlite_table = options["ext-login-sqlite-table"].as<string>();

      my->login_strat = std::make_unique<sqlite_login>(
         my->sqlite_db_path,
         my->sqlite_table
         );
   }
   else // my->use_mariadb
   {
      if( options.count( "ext-login-mariadb-host" ) )
         my->mariadb_host = options["ext-login-mariadb-host"].as<string>();
      if( options.count( "ext-login-mariadb-port" ) )
         my->mariadb_port = options["ext-login-mariadb-port"].as<uint32_t>();
      if( options.count( "ext-login-mariadb-user" ) )
         my->mariadb_user = options["ext-login-mariadb-user"].as<string>();
      if( options.count( "ext-login-mariadb-pass" ) )
         my->mariadb_pass = options["ext-login-mariadb-pass"].as<string>();
      if( options.count( "ext-login-mariadb-db-name" ) )
         my->mariadb_db_name = options["ext-login-mariadb-db-name"].as<string>();
      if( options.count( "ext-login-mariadb-table") )
         my->mariadb_table = options["ext-login-mariadb-table"].as<string>();

      my->login_strat = std::make_unique<mariadb_login>(
         my->mariadb_host,
         my->mariadb_port,
         my->mariadb_user,
         my->mariadb_pass,
         my->mariadb_db_name,
         my->mariadb_table,
         my->mariadb_unix_sock
         );
   }

   app().login_attempt.connect(
         [this]( const string& user, const string& password, login_api& login_api ) {
            my->on_login( user, password, login_api );
         }
      );
}

void external_login_plugin::plugin_startup()
{
}

} }
