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
#include <graphene/zeromq/zeromq_plugin.hpp>
#include <graphene/app/api.hpp>

#include "../common/database_fixture.hpp"

#define BOOST_TEST_MODULE ZeroMQ Plugin Test
#include <boost/test/included/unit_test.hpp>

using namespace fc;
using namespace graphene::app;
using namespace graphene::zeromq;
using namespace graphene::chain;
using namespace graphene::chain::test;
namespace bpo = boost::program_options;

struct zeromq_fixture : public database_fixture
{
    zeromq_fixture()
    {
        try 
        {
            app.register_plugin<zeromq_plugin>( true );

            app.initialize_plugins( bpo::variables_map() ); 
        } 
        catch(fc::exception &e)
        {
            edump((e.to_detail_string() ));
        }
    }
};

BOOST_FIXTURE_TEST_SUITE( zeromq_tests, zeromq_fixture )

BOOST_AUTO_TEST_CASE( demo )
{ try {

   ACTORS( (alice) (bob) );
   transfer( committee_account, alice_id, asset(10000) );
   transfer( committee_account, bob_id, asset(10000) );
   generate_block();

   try {
      app.register_plugin<graphene::zeromq::zeromq_plugin>( true );
      app.initialize_plugins( bpo::variables_map() );
   } catch( fc::exception &e ) {
      edump( (e.to_detail_string() ) );
   }

   transfer( alice, bob, asset(1) );
   generate_block();

   transfer( alice, bob, asset(2) );
   transfer( bob, alice, asset(3) );
   generate_block();

   transfer( alice, bob, asset(4) );
   transfer( alice, bob, asset(5) );
   generate_block();

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()