#include <graphene/app/database_api.hpp>
#include <graphene/mobile/mobile_subscriptions.hpp>
#include <graphene/mobile/mobile_account.hpp>
#include <fc/bloom_filter.hpp>

namespace graphene { namespace mobile {

   namespace detail {

      class subscriptions_impl : public std::enable_shared_from_this<subscriptions_impl>
      {
         public:
            subscriptions_impl(graphene::chain::database& db);
            void set_subscribe_callback( std::function<void(const variant&)> cb );
            void subscribe_to_account(account_id_type account_id);

         private:
            graphene::chain::database& _db;

            void broadcast_updates( const vector<variant>& updates );
            void handle_object_changed(bool force_notify,
                  bool full_object,
                  const vector<object_id_type>& ids,
                  const flat_set<account_id_type>& impacted_accounts,
                  std::function<const object*(object_id_type id)> find_object);

            /** called every time a block is applied to report the objects that were changed */
            void on_objects_new(const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts);
            void on_objects_changed(const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts);
            void on_applied_block();

            boost::signals2::scoped_connection _new_connection;
            boost::signals2::scoped_connection _change_connection;
            boost::signals2::scoped_connection _applied_block_connection;

            /// The callback for subscriptions
            std::set<std::vector<char>> _subscribed_ids;
            std::set<account_id_type> _subscribed_accounts;
            std::function<void(const fc::variant&)> _subscribe_callback;

            bool is_impacted_account( const flat_set<account_id_type>& accounts)
            {
               if( !_subscribed_accounts.size() || !accounts.size() )
                  return false;

               return std::any_of(
                  accounts.begin(),
                  accounts.end(),
                  [this](const account_id_type& account) {
                     return _subscribed_accounts.find(account) != _subscribed_accounts.end();
                  });
            }

      };

      subscriptions_impl::subscriptions_impl( graphene::chain::database& _db ) :
         _db( _db )
      {
         wlog("mobile::subscription constructor");
         /**
          *  Emitted After a block has been applied and committed.  The callback
          *  should not yield and should execute quickly.
          */
         _new_connection = _db.new_objects.connect([this](const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts) {
                                      on_objects_new(ids, impacted_accounts);
                                      });
         /**
          *  Emitted After a block has been applied and committed.  The callback
          *  should not yield and should execute quickly.
          */
         _change_connection = _db.changed_objects.connect([this](const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts) {
                                      on_objects_changed(ids, impacted_accounts);
                                      });

         /**
          *  This signal is emitted after all operations and virtual operation for a
          *  block have been applied but before the get_applied_operations() are cleared.
          *
          *  You may not yield from this callback because the blockchain is holding
          *  the write lock and may be in an "inconstant state" until after it is
          *  released.
          */
         _applied_block_connection = _db.applied_block.connect([this](const signed_block&){
                                    on_applied_block(); });
      }

      void subscriptions_impl::set_subscribe_callback( std::function<void(const variant&)> cb )
      {
         _subscribe_callback = cb;

         _subscribed_accounts.clear();
      }

      void subscriptions_impl::subscribe_to_account(account_id_type account_id)
      {
         if( _subscribe_callback )
         {
            FC_ASSERT( _subscribed_accounts.size() <= 100 );
            _subscribed_accounts.insert( account_id );
            wlog("Enabled subscription for account ${id}", ("id", account_id));
         }
      }

      void subscriptions_impl::on_objects_new(const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts)
      {
         handle_object_changed(
               false,
               true,
               ids,
               impacted_accounts,
               std::bind(&object_database::find_object, &_db, std::placeholders::_1)
               );
      }

      void subscriptions_impl::on_objects_changed(const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts)
      {
         handle_object_changed(
               false,
               true,
               ids,
               impacted_accounts,
               std::bind(&object_database::find_object, &_db, std::placeholders::_1)
               );
      }

      void subscriptions_impl::handle_object_changed(
            bool force_notify,
            bool full_object,
            const vector<object_id_type>& ids,
            const flat_set<account_id_type>& impacted_accounts,
            std::function<const object*(object_id_type id)> find_object)
      {
         if( _subscribe_callback )
         {
            vector<variant> updates;

            for(auto id : ids)
            {
               // wlog("Subscribed to id ${id}", ("id", id));
               if( force_notify || is_impacted_account(impacted_accounts) )
               {
                  // account_balance_id_type
                  if( id.is<account_balance_id_type>() )
                  {
                     auto obj = find_object(id);
                     if( obj )
                     {
                        auto balance = dynamic_cast<const account_balance_object*>(obj);
                        mobile_balance_update_object updated_balance;
                        asset_object balance_asset = _db.get(balance->asset_type);
                        mobile_account_balance_object mobile_balance;
                        mobile_balance.balance = balance->balance;
                        mobile_balance.precision = balance_asset.precision;
                        mobile_balance.symbol = balance_asset.symbol;
                        updated_balance.new_balance = mobile_balance;

                        account_object owner = _db.get(balance->owner);
                        updated_balance.name = owner.name;

                        updates.emplace_back( updated_balance.to_variant() );
                     }
                  }
               }
            }
            broadcast_updates(updates);
         }
      }

      void subscriptions_impl::broadcast_updates( const vector<variant>& updates )
      {
         if( updates.size() && _subscribe_callback ) {
            auto capture_this = shared_from_this();
            fc::async([capture_this,updates](){
                if(capture_this->_subscribe_callback)
                  capture_this->_subscribe_callback( fc::variant(updates) );
            });
         }
      }

      void subscriptions_impl::on_applied_block()
      {
         if (_subscribe_callback)
         {
            auto capture_this = shared_from_this();
            uint32_t block_num = _db.head_block_num();
            fc::async([this,capture_this, block_num](){
               _subscribe_callback(fc::variant(block_num));
            });
         }
      }


   } /// end of detail namespace

   subscriptions::subscriptions(graphene::chain::database& db) :
      my( new detail::subscriptions_impl( db ) )
   {}
   subscriptions::~subscriptions() {}

   void subscriptions::set_subscribe_callback( std::function<void(const variant&)> cb )
   {
      my->set_subscribe_callback( cb );
   }

   void subscriptions::subscribe_to_account(account_id_type account_id)
   {
      my->subscribe_to_account(account_id);
   }

}}
