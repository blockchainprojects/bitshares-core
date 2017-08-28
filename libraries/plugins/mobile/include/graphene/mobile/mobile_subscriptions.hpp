#pragma once

#include <graphene/chain/database.hpp>

namespace graphene { namespace mobile {

   using namespace graphene::chain;

   namespace detail {
      class subscriptions_impl;
   }

   class subscriptions
   {
      public:
         subscriptions(graphene::chain::database& db);
         ~subscriptions();
         void set_subscribe_callback( std::function<void(const variant&)> cb );
         void subscribe_to_account(account_id_type account_id);

      private:
         std::shared_ptr< detail::subscriptions_impl > my;
   };
}}
