/*
 * Copyright (c) 2018 oxarbitrage and contributors.
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

#include <zmq.hpp>

#include <graphene/chain/impacted.hpp>
#include <graphene/chain/account_evaluator.hpp>
#include <fc/smart_ref_impl.hpp>

#include <iterator>
#include <memory>

namespace graphene { namespace zeromq {

namespace detail
{

class zeromq_plugin_impl
{
   public:
      zeromq_plugin_impl(zeromq_plugin& _plugin)
         : _self( _plugin )
      {
         _socket = std::unique_ptr<zmq::socket_t>( new zmq::socket_t(_ctx, ZMQ_PUB) );
      }
      ~zeromq_plugin_impl();


      void on_applied_block( const signed_block& b );

      graphene::chain::database& database()
      {
         return _self.database();
      }

      zeromq_plugin& _self;

      string _endpoint = "tcp://127.0.0.1:5556";
      
      std::unique_ptr<zmq::socket_t> _socket;
   private:
      zmq::context_t _ctx;

      template<typename T>
      bool add_zeromq( message_type msg_type, const T& var );
};

void zeromq_plugin_impl::on_applied_block( const signed_block& b )
{
   // TODO REMOVE ONLY FOR TEST
   static message_type type = message_type::block;
   type = type==message_type::block ? message_type::sample : message_type::block; 
      
   add_zeromq<signed_block>( type, b );
}

template<typename T> 
bool zeromq_plugin_impl::add_zeromq( message_type msg_type, const T& var )
{
   string type = fc::to_string( msg_type );
   string json = std::move( fc::json::to_string<T>(var) );

   /*
   zmq::message_t zmq_message( type.length() + json.length() );
   auto* pmsg = (unsigned char*) zmq_message.data();
   memcpy( pmsg, type.c_str(), type.length() );
   pmsg += type.length();
   memcpy( pmsg, json.c_str(), json.length() + 1 );
   */

   string message( std::move( fc::to_string( msg_type ) + fc::json::to_string<T>(var) + "\0" ) );
   edump( (message) ); // TODO
   zmq::message_t zmq_message( message.c_str(), message.length() );
   memcpy( zmq_message.data(), message.c_str(), message.length() );

   _socket->send( zmq_message );
   return true;
}

zeromq_plugin_impl::~zeromq_plugin_impl()
{
   return;
}

} // end namespace detail

zeromq_plugin::zeromq_plugin() :
   my( new detail::zeromq_plugin_impl(*this) )
{
}

zeromq_plugin::~zeromq_plugin()
{
}

std::string zeromq_plugin::plugin_name()const
{
   return "zeromq";
}
std::string zeromq_plugin::plugin_description()const
{
   return "Stores account history data in zeromq database(EXPERIMENTAL).";
}

void zeromq_plugin::plugin_set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg
   )
{
   cli.add_options()
         ("endpoint", boost::program_options::value<std::string>(), "zeromq socket (tcp://127.0.0.1:5556)")
         ;
   cfg.add(cli);
}

void zeromq_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   auto& db = database();
   
   

   if (options.count("endpoint")) {
      my->_endpoint = options["endpoint"].as<std::string>();
   }
}

void zeromq_plugin::plugin_startup()
{
   ilog("Binding to ${u}", ("u", my->_endpoint));
   my->_socket->bind( my->_endpoint );

   database().applied_block.connect( [&]( const signed_block& b) {
      my->on_applied_block(b); });
}

} }
