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

#pragma once
#include <graphene/external_login/login_strategy.hpp>
#include <graphene/external_login/sql_helper.hpp>

#include <mariadb++/connection.hpp>
#include <mariadb++/account.hpp>

namespace graphene { namespace external_login {

class mariadb_login : public login_strategy
{
public:
   mariadb_login( const std::string& host, const uint32_t port, const std::string& user, const std::string& pass,
      const std::string& db_name, const std::string& table, const std::string& unix_sock
      );

   fc::optional<api_access_info> get_api_access_info( const std::string& user ) const override;

private:
   mariadb::connection_ref _connection;
   mariadb::account_ref    _account_setup;

   std::string _table;
};

}} // graphene::external_login