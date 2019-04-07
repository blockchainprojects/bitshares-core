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

zmq::context_t ctx;
zmq::socket_t s(ctx, ZMQ_PUSH);

namespace graphene { namespace zeromq {

namespace detail
{

class zeromq_plugin_impl
{
   public:
      zeromq_plugin_impl(zeromq_plugin& _plugin)
         : _self( _plugin )
      {
      }
      virtual ~zeromq_plugin_impl();

      bool update_account_histories( const signed_block& b);


      graphene::chain::database& database()
      {
         return _self.database();
      }

      zeromq_plugin& _self;

      std::string _zeromq_socket = "tcp://127.0.0.1:5556";

      send_struct zeromq_struct;

   private:

      bool add_zeromq();
};

void zeromq_plugin_impl::on_applied_block( const signed_block& b )
{
   edump( (b.to_string()) );
}

bool zeromq_plugin_impl::add_zeromq( const account_id_type account_id,
                                     const optional <operation_history_object>& oho)
{
   const auto &stats_obj = getStatsObject(account_id);
   const auto &ath = addNewEntry(stats_obj, account_id, oho);
   growStats(stats_obj, ath);
   cleanObjects(ath.id, account_id);

   // here we have everything?
   zeromq_struct.account_history = ath;
   zeromq_struct.operation_history = os;
   zeromq_struct.operation_type = op_type;
   zeromq_struct.operation_id_num = ath.operation_id.instance.value;
   zeromq_struct.block_data = bs;

   string my_json = fc::json::to_string(zeromq_struct);

   int32_t msgtype = 0;
   int32_t msgopts = 0;

   zmq::message_t message(my_json.length()+sizeof(msgtype)+sizeof(msgopts));
   unsigned char* ptr = (unsigned char*) message.data();
   memcpy(ptr, &msgtype, sizeof(msgtype));
   ptr += sizeof(msgtype);
   memcpy(ptr, &msgopts, sizeof(msgopts));
   ptr += sizeof(msgopts);
   memcpy(ptr, my_json.c_str(), my_json.length());
   s.send(message);

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
         ("zeromq-socket", boost::program_options::value<std::string>(), "zeromq socket (tcp://127.0.0.1:5556)")
         ;
   cfg.add(cli);
}

void zeromq_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   auto& db = database();
   
   

   if (options.count("zeromq-socket")) {
      my->_zeromq_socket = options["zeromq-socket"].as<std::string>();
   }
}

void zeromq_plugin::plugin_startup()
{
   ilog("Binding to ${u}", ("u", my->_zeromq_socket));
   s.bind (my->_zeromq_socket);

   database().applied_block.connect( [&]( const signed_block& b) {
      if(!my->on_applied_block(b))
      {
         FC_THROW_EXCEPTION(graphene::chain::plugin_exception, "Error populating ES database, we are going to keep trying.");
      }
   } );
}

} }
