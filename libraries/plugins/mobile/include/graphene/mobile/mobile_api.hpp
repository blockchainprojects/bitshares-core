
#pragma once

#include <fc/api.hpp>
#include <graphene/mobile/mobile_account.hpp>

/*
 * We need this so we can refer to graphene::app::application without including the entire header
 */
namespace graphene { namespace app {
class application;
} }

/*
 * Our basic namespace
 */
namespace graphene { namespace mobile {

namespace detail {
 /* The "detail" is used to namespace the actual implemention and separated it
  * from the interface
  */
 class mobile_api_impl;
}

class mobile_api
{
   public:
      /**
       * The API requires a constructor which takes app.
       */
      mobile_api( graphene::app::application& app );

      /**
       * Called immediately after the constructor. If the API class uses enable_shared_from_this,
       * shared_from_this() is available in this method, which allows signal handlers to be registered
       * with app::connect_signal()
       */
      void on_api_startup();

      /**
       * @brief Fetch a reduced set of information relevant to the specified accounts
       * @param names_or_ids Each item must be the name or ID of an account to retrieve
       * @return Map of string from @ref names_or_ids to the corresponding account
       *
       * This function fetches a reduced set of information relevant for the given accounts.
       * If any of the strings in @ref names_or_ids cannot be tied to an account, that input
       * will be ignored.  All other accounts will be retrieved.
       *
       */
      std::map<string,mobile_account> get_mobile_accounts( const vector<string>& names_or_ids );

      /**
       * @brief Fetch the balances of an account
       * @param names_or_ids Each item must be the name or ID of an account to retrieve
       * @return Vector of balance describing objects
       *
       * This function fetches a mobile-friendly version of each
       * balance/asset in an account.
       *
       */
      vector<mobile_account_balance_object> get_mobile_balances( const string& name_or_id );

      /**
       * @brief Obtain a list of accounts that use a given public key
       * @param key Vector of public key Base58Check encoded, e.g. BTS54ygSazvgwXVVFaMfutf9BxVatchAvxaVwscTxjikeeJBQmrVt
       * @return Vector of account ids per public key
       *
       */
      vector<vector<account_id_type>> get_mobile_key_references( vector<public_key_type> key )const;

      /**
       * @brief set a subscription callback
       * @param callback
       *
       */
      void set_subscribe_callback( std::function<void(const variant&)> cb );

   private:
      /*
       * The implementation instance is stored privately in `my`
       */
      std::shared_ptr< detail::mobile_api_impl > my;
};

} }

/*
 * Reflection for C++ allowing automatic serialization in Json & Binary of C++
 * Structs 
 */
FC_API( graphene::mobile::mobile_api,
    (get_mobile_accounts)
    (get_mobile_balances)
    (get_mobile_key_references)
    (set_subscribe_callback)
)
