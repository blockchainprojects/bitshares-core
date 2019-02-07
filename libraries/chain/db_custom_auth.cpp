/*
 * Copyright (c) 2019 BitShares Core Team, and contributors.
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

#include <graphene/chain/database.hpp>

#include <graphene/chain/custom_authority_object.hpp>
#include <graphene/chain/exceptions.hpp>

namespace graphene { namespace chain {

namespace {
   vector< custom_authority_object > filter_enabled_custom_authorities( const vector< custom_authority_object >& custom_authorities )
   {
      vector< custom_authority_object > result;
      for (auto& auth: custom_authorities)
      {
         if (auth.enabled)
         {
            result.emplace_back(auth);
         }
      }
      
      return result;
   }
   
   flat_set< account_id_type > get_required_accounts( const operation& op )
   {
      flat_set<account_id_type> required_accounts;
      
      flat_set<account_id_type> active_accounts;
      flat_set<account_id_type> owner_accounts;
      
      //it is need only as argument for operation_get_required_authorities() function
      //we won't use it anyhow
      vector<authority> other_authorities;
      
      operation_get_required_authorities(op, active_accounts, owner_accounts, other_authorities);
      
      required_accounts.insert(active_accounts.begin(), active_accounts.end());
      required_accounts.insert(owner_accounts.begin(), owner_accounts.end());
      
      return required_accounts;
   }
}

vector< custom_authority_object > database::get_custom_authorities_by_account( account_id_type account ) const
{
   const auto& authority_by_account = get_index_type<custom_authority_index>().indices().get<by_account>();
   
   vector<custom_authority_object> result;
   
   auto itr = authority_by_account.find(account);
   while(itr != authority_by_account.end() && (*itr).account == account)
   {
      result.emplace_back(*itr++);
   }
   
   return result;
}
   
void database::verify_custom_authorities( const transaction& trx )const
{
   for (auto& op: trx.operations)
   {
      auto required_accounts = get_required_accounts(op);
      for (auto& account_id: required_accounts)
      {
         auto custom_authorities = get_custom_authorities_by_account(account_id);
         custom_authorities = filter_enabled_custom_authorities(custom_authorities);
         
         bool operation_verified = custom_authorities.empty();
         for (auto& custom_auth: custom_authorities)
         {
            operation_verified |= custom_auth.validate(op, head_block_time());
         }
         
         FC_ASSERT(operation_verified, "Operation was not verified by any custom authority.");
      }
   }
}

} }
