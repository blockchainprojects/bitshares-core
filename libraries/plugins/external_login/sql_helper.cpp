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

#include <graphene/external_login/sql_helper.hpp>

#include <algorithm>
#include <sstream>

using std::string;
using std::stringstream;
using std::vector;

namespace graphene { namespace external_login {

const string sql_helper::make_query( const string& table, const string& user )
{
   return "SELECT password_hash_b64, password_salt_b64, allowed_apis FROM " + table + " where username='" + user + "'";
}

vector<string> sql_helper::allowed_apis_from_string( string allowed_apis_str )
{
   vector<string> allowed_apis;

   // format can contain ' ' replace them with ',' for better seperation
   std::replace( allowed_apis_str.begin(), allowed_apis_str.end(), ' ', ',' );

   stringstream allowed_apis_ss( allowed_apis_str );
   string in;
   while( getline( allowed_apis_ss, in, ',' ) ) {
      if( !in.empty() )
         allowed_apis.emplace_back( move(in) );
   }

   return allowed_apis;
}

}} // graphene::external_login
