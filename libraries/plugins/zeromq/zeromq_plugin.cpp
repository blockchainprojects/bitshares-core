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
      primary_index< operation_history_index >* _oho_index;
      int16_t op_type;
      operation_history_struct os;
      block_struct bs;

   private:

      bool add_zeromq( const account_id_type account_id, const optional<operation_history_object>& oho );
      const account_transaction_history_object& addNewEntry(const account_statistics_object& stats_obj,
                                                            const account_id_type& account_id,
                                                            const optional <operation_history_object>& oho);
      const account_statistics_object& getStatsObject(const account_id_type& account_id);
      void growStats(const account_statistics_object& stats_obj, const account_transaction_history_object& ath);
      void getOperationType(const optional <operation_history_object>& oho);
      void doOperationHistory(const optional <operation_history_object>& oho);
      void doBlock(uint32_t trx_in_block, const signed_block& b);
      void cleanObjects(const account_transaction_history_id_type& ath, const account_id_type& account_id);
};


bool zeromq_plugin_impl::update_account_histories( const signed_block& b )
{

   graphene::chain::database& db = database();
   const vector<optional< operation_history_object > >& hist = db.get_applied_operations();
   bool is_first = true;
   auto skip_oho_id = [&is_first,&db,this]() 
   {
      if( is_first && db._undo_db.enabled() ) // this ensures that the current id is rolled back on undo
      {
         db.remove( db.create<operation_history_object>( []( operation_history_object& obj) {} ) );
         is_first = false;
      }
      else
         _oho_index->use_next_id();
   };

   for( const optional< operation_history_object >& o_op : hist ) 
   {
      optional <operation_history_object> oho;

      auto create_oho = [&]() {
         is_first = false;
         return optional<operation_history_object>(
               db.create<operation_history_object>([&](operation_history_object &h) {
                  if (o_op.valid())
                  {
                     h.op           = o_op->op;
                     h.result       = o_op->result;
                     h.block_num    = o_op->block_num;
                     h.trx_in_block = o_op->trx_in_block;
                     h.op_in_trx    = o_op->op_in_trx;
                     h.virtual_op   = o_op->virtual_op;
                  }
               }));
      };

      if( !o_op.valid() ) {
         skip_oho_id();
         continue;
      }
      oho = create_oho();

      // populate what we can before impacted loop
      getOperationType(oho);
      doOperationHistory(oho);
      doBlock(oho->trx_in_block, b);

      const operation_history_object& op = *o_op;

      // get the set of accounts this operation applies to
      flat_set<account_id_type> impacted;
      vector<authority> other;
      operation_get_required_authorities( op.op, impacted, impacted, other ); // fee_payer is added here

      if( op.op.which() == operation::tag< account_create_operation >::value )
         impacted.insert( op.result.get<object_id_type>() );
      else
         graphene::chain::operation_get_impacted_accounts( op.op, impacted );

      for( auto& a : other )
         for( auto& item : a.account_auths )
            impacted.insert( item.first );

      for( auto& account_id : impacted )
      {
         if(!add_zeromq( account_id, oho ))
            return false;
      }
   }
   return true;
}


void zeromq_plugin_impl::getOperationType(const optional <operation_history_object>& oho)
{
   if (!oho->id.is_null())
      op_type = oho->op.which();
}

void zeromq_plugin_impl::doOperationHistory(const optional <operation_history_object>& oho)
{
   os.trx_in_block = oho->trx_in_block;
   os.op_in_trx = oho->op_in_trx;
   os.operation_results = oho->result;
   os.virtual_op = oho->virtual_op;
   os.op = oho->op;
}

void zeromq_plugin_impl::doBlock(uint32_t trx_in_block, const signed_block& b)
{
   std::string trx_id = "";
   if(trx_in_block < b.transactions.size())
      trx_id = b.transactions[trx_in_block].id().str();
   bs.block_num = b.block_num();
   bs.block_time = b.timestamp;
   bs.trx_id = trx_id;
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

const account_statistics_object& zeromq_plugin_impl::getStatsObject(const account_id_type& account_id)
{
   graphene::chain::database& db = database();
   const auto &stats_obj = db.get_account_stats_by_owner(account_id);

   return stats_obj;
}

const account_transaction_history_object& zeromq_plugin_impl::addNewEntry(const account_statistics_object& stats_obj,
                                                                                 const account_id_type& account_id,
                                                                                 const optional <operation_history_object>& oho)
{
   graphene::chain::database& db = database();
   const auto &ath = db.create<account_transaction_history_object>([&](account_transaction_history_object &obj) {
      obj.operation_id = oho->id;
      obj.account = account_id;
      obj.sequence = stats_obj.total_ops + 1;
      obj.next = stats_obj.most_recent_op;
   });

   return ath;
}

void zeromq_plugin_impl::growStats(const account_statistics_object& stats_obj,
                                          const account_transaction_history_object& ath)
{
   graphene::chain::database& db = database();
   db.modify(stats_obj, [&](account_statistics_object &obj) {
      obj.most_recent_op = ath.id;
      obj.total_ops = ath.sequence;
   });
}


void zeromq_plugin_impl::cleanObjects(const account_transaction_history_id_type& ath_id, const account_id_type& account_id)
{
   graphene::chain::database& db = database();
   // remove everything except current object from ath
   const auto &his_idx = db.get_index_type<account_transaction_history_index>();
   const auto &by_seq_idx = his_idx.indices().get<by_seq>();
   auto itr = by_seq_idx.lower_bound(boost::make_tuple(account_id, 0));
   if (itr != by_seq_idx.end() && itr->account == account_id && itr->id != ath_id) {
      // if found, remove the entry
      const auto remove_op_id = itr->operation_id;
      const auto itr_remove = itr;
      ++itr;
      db.remove( *itr_remove );
      // modify previous node's next pointer
      // this should be always true, but just have a check here
      if( itr != by_seq_idx.end() && itr->account == account_id )
      {
         db.modify( *itr, [&]( account_transaction_history_object& obj ){
            obj.next = account_transaction_history_id_type();
         });
      }
      // do the same on oho
      const auto &by_opid_idx = his_idx.indices().get<by_opid>();
      if (by_opid_idx.find(remove_op_id) == by_opid_idx.end()) {
         db.remove(remove_op_id(db));
      }
   }
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

   my->_oho_index = database().add_index< primary_index< operation_history_index > >();
   database().add_index< primary_index< account_transaction_history_index > >();

   ilog("Binding to ${u}", ("u", my->_zeromq_socket));
   s.bind (my->_zeromq_socket);

   database().applied_block.connect( [&]( const signed_block& b) {
      if(!my->update_account_histories(b))
      {
         FC_THROW_EXCEPTION(graphene::chain::plugin_exception, "Error populating ES database, we are going to keep trying.");
      }
   } );

   if (options.count("zeromq-socket")) {
      my->_zeromq_socket = options["zeromq-socket"].as<std::string>();
   }
}

void zeromq_plugin::plugin_startup()
{

}

} }
