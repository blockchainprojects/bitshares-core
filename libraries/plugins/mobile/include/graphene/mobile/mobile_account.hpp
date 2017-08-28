#pragma once

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>

namespace graphene {
namespace mobile {

   using namespace graphene::chain;

   /**
    * @brief Tracks the balance of a single account/asset pair
    * @ingroup object
    */
   struct mobile_account_object
   {
      string            name;
      authority         owner;
      authority         active;
      account_options   options;
   };

   struct mobile_account_balance_object
   {
      share_type        balance;
      string            symbol;
      uint8_t           precision = 0;
   };

   struct mobile_account
   {
      mobile_account_object                   account;
      vector<mobile_account_balance_object>   balances;
   };

   /*
    * Updates of mobile account balances
    */
   // Inherits abstract_object so we can use to_variant()
   class mobile_balance_update_object : public graphene::db::abstract_object<mobile_balance_update_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = 149;

         string            name;
         mobile_account_balance_object    new_balance;
   };

}}

FC_REFLECT( graphene::mobile::mobile_account,
            (account)
            (balances)
          );
FC_REFLECT( graphene::mobile::mobile_account_object,
            (name)(owner)(active)(options)
          );

FC_REFLECT( graphene::mobile::mobile_account_balance_object,
            (balance)(symbol)(precision)
          );

FC_REFLECT( graphene::mobile::mobile_balance_update_object,
            (name)(new_balance)
          );
