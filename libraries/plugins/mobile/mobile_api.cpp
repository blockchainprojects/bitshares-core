#include <graphene/app/application.hpp>

#include <graphene/chain/database.hpp>

#include <graphene/mobile/mobile_api.hpp>
#include <graphene/mobile/mobile_plugin.hpp>
#include <graphene/mobile/mobile_account.hpp>
#include <graphene/mobile/mobile_subscriptions.hpp>

namespace graphene { namespace mobile {

/*
 * The detail is used to separated the implementation from the interface
 */
namespace detail {

   class mobile_api_impl
   {
      public:
         mobile_api_impl( graphene::app::application& _app );
         graphene::app::application& app;
         std::shared_ptr< graphene::mobile_plugin::mobile_plugin > get_plugin();

         void set_subscribe_callback( std::function<void(const variant&)> cb );
         std::map<string,mobile_account> get_mobile_accounts( const vector<string>& names_or_ids );
         vector<mobile_account_balance_object> get_mobile_balances( const string& name_or_id );
         vector<vector<account_id_type>> get_mobile_key_references( vector<public_key_type> key )const;

         graphene::chain::database& _db;
         std::shared_ptr<graphene::mobile::subscriptions> _subscriptions;
   };

   /**
    * Constructor Implementation
    */
   mobile_api_impl::mobile_api_impl( graphene::app::application& _app ) :
      app( _app ),
      _db( *_app.chain_database() ),
      _subscriptions( new graphene::mobile::subscriptions( *_app.chain_database() ) )
   {}

   /**
    * Get plugin name Implementation
    */
   std::shared_ptr< graphene::mobile_plugin::mobile_plugin > mobile_api_impl::get_plugin()
   {
      /// We are loading the mobile_plugin's get_plugin all here
      return app.get_plugin< graphene::mobile_plugin::mobile_plugin >( "mobile" );
   }

   /**
    * Custom call call implementations
    */
   std::map<std::string, mobile_account> mobile_api_impl::get_mobile_accounts( const vector<std::string>& names_or_ids)
   {
      idump((names_or_ids));
      std::map<std::string, mobile_account> results;

      for (const std::string& account_name_or_id : names_or_ids)
      {
         const account_object* account = nullptr;
         if (std::isdigit(account_name_or_id[0]))
            account = _db.find(fc::variant(account_name_or_id).as<account_id_type>());
         else
         {
            const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
            auto itr = idx.find(account_name_or_id);
            if (itr != idx.end())
               account = &*itr;
         }
         if (account == nullptr)
            continue;

         // Subscribe to this account
         _subscriptions->subscribe_to_account(account->get_id());

         // Account
         mobile_account acnt;

         // Account itself
         mobile_account_object account_object;
         account_object.name = account->name;
         account_object.owner = account->owner;
         account_object.active = account->active;
         account_object.options = account->options;
         acnt.account = account_object;

         // Add the account's balances
         auto balance_range = _db.get_index_type<account_balance_index>().indices().get<by_account_asset>().equal_range(boost::make_tuple(account->id));
         //vector<account_balance_object> balances;
         std::for_each(balance_range.first, balance_range.second,
                       [&acnt, this](const account_balance_object& balance) {
                          // get corresponding asset
                          asset_object balance_asset = _db.get(balance.asset_type);
                          // construct mobile account balance object
                          mobile_account_balance_object mobile_balance;
                          mobile_balance.balance = balance.balance;
                          mobile_balance.precision = balance_asset.precision;
                          mobile_balance.symbol = balance_asset.symbol;
                          acnt.balances.emplace_back(mobile_balance);
                       });
         results[account_name_or_id] = acnt;
      }
      return results;
   }

   vector<mobile_account_balance_object> mobile_api_impl::get_mobile_balances( const string& name_or_id )
   {
      // Return object
      vector<mobile_account_balance_object> balances;

      const account_object* account = nullptr;
      if (std::isdigit(name_or_id[0]))
         account = _db.find(fc::variant(name_or_id).as<account_id_type>());
      else
      {
         const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
         auto itr = idx.find(name_or_id);
         if (itr != idx.end())
            account = &*itr;
      }
      if (account == nullptr)
         return balances;

      // Add the account's balances
      auto balance_range = _db.get_index_type<account_balance_index>().indices().get<by_account_asset>().equal_range(boost::make_tuple(account->id));
      //vector<account_balance_object> balances;
      std::for_each(balance_range.first, balance_range.second,
                    [&balances, this](const account_balance_object& balance) {
                       // get corresponding asset
                       asset_object balance_asset = _db.get(balance.asset_type);
                       // construct mobile account balance object
                       mobile_account_balance_object mobile_balance;
                       mobile_balance.balance = balance.balance;
                       mobile_balance.precision = balance_asset.precision;
                       mobile_balance.symbol = balance_asset.symbol;
                       balances.emplace_back(mobile_balance);
                    });
      return balances;
   }

   vector<vector<account_id_type>> mobile_api_impl::get_mobile_key_references( vector<public_key_type> keys )const
   {
      wdump( (keys) );
      vector< vector<account_id_type> > final_result;
      final_result.reserve(keys.size());

      for( auto& key : keys )
      {
         address a1( pts_address(key, false, 56) );
         address a2( pts_address(key, true, 56) );
         address a3( pts_address(key, false, 0)  );
         address a4( pts_address(key, true, 0)  );
         address a5( key );

         const auto& idx = _db.get_index_type<account_index>();
         const auto& aidx = dynamic_cast<const primary_index<account_index>&>(idx);
         const auto& refs = aidx.get_secondary_index<graphene::chain::account_member_index>();
         auto itr = refs.account_to_key_memberships.find(key);
         vector<account_id_type> result;

         for( auto& a : {a1,a2,a3,a4,a5} )
         {
             auto itr = refs.account_to_address_memberships.find(a);
             if( itr != refs.account_to_address_memberships.end() )
             {
                result.reserve( itr->second.size() );
                for( auto item : itr->second )
                {
                   wdump((a)(item)(item(_db).name));
                   result.push_back(item);
                }
             }
         }

         if( itr != refs.account_to_key_memberships.end() )
         {
            result.reserve( itr->second.size() );
            for( auto item : itr->second ) result.push_back(item);
         }
         final_result.emplace_back( std::move(result) );
      }

      return final_result;
   }

   void mobile_api_impl::set_subscribe_callback( std::function<void(const variant&)> cb )
   {
      _subscriptions->set_subscribe_callback(cb);
   }


} /// detail

/*
 * Now we are done with implementation/detail, let's do the interface (linking
 * implementation)
 */

/*
 * Plugin constructor implementation
 */
mobile_api::mobile_api( graphene::app::application& app )
   : my( new detail::mobile_api_impl( app ) ) {}

/*
 * Custom call 'mobile' interface
 */
std::map<string,mobile_account> mobile_api::get_mobile_accounts( const vector<string>& names_or_ids )
{
   return my->get_mobile_accounts( names_or_ids );
}

vector<mobile_account_balance_object> mobile_api::get_mobile_balances( const string& name_or_id )
{
   return my->get_mobile_balances( name_or_id );
}

vector<vector<account_id_type>> mobile_api::get_mobile_key_references( vector<public_key_type> key )const
{
   return my->get_mobile_key_references( key );
}

void mobile_api::set_subscribe_callback( std::function<void(const variant&)> cb)
{
   my->set_subscribe_callback( cb );
}

} } // graphene::mobile
