/*
 * Copyright (c) 2018 Abit More, and contributors.
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
#include <graphene/chain/protocol/custom_authority.hpp>
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/int_from_operation_type.hpp>

namespace graphene { namespace chain {

namespace {
   template <typename Operation>
   struct restriction_validator_visitor
   {
      typedef void result_type;
      
      template <typename Restriction>
      void operator () (const Restriction& rest) const
      {
         rest.template validate<Operation>();
      }
   };
   
   struct operation_type_checker
   {
      operation_type_checker(const restriction& a_rest)
      : rest(a_rest)
      {}
      
      template <typename Operation>
      void operator () () const
      {
         restriction_validator_visitor<Operation> visitor;
         rest.visit(visitor);
      }
      
      const restriction& rest;
   };
}
   
share_type custom_authority_create_operation::calculate_fee( const fee_parameters_type& k )const
{
   share_type core_fee_required = k.basic_fee;

   if( enabled )
   {
      share_type unit_fee = k.price_per_k_unit;
      unit_fee *= (valid_to - valid_from).to_seconds();
      unit_fee *= auth.num_auths();
      uint64_t restriction_units = 0;
      for( const auto& r : restrictions )
      {
         units_fetcher visitor;
         restriction_units += r.visit(visitor);
      }
      unit_fee *= restriction_units;
      unit_fee /= 1000;
      core_fee_required += unit_fee;
   }

   return core_fee_required;
}

void custom_authority_create_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0, "Fee amount can not be negative" );

   FC_ASSERT( account != GRAPHENE_TEMP_ACCOUNT
              && account != GRAPHENE_COMMITTEE_ACCOUNT
              && account != GRAPHENE_WITNESS_ACCOUNT
              && account != GRAPHENE_RELAXED_COMMITTEE_ACCOUNT,
              "Can not create custom authority for special accounts" );

   FC_ASSERT( valid_from < valid_to, "valid_from must be earlier than valid_to" );
   
   for( const auto& r : restrictions )
   {
      operation_type_from_int(operation_type, operation_type_checker(r));
   }
}

share_type custom_authority_update_operation::calculate_fee( const fee_parameters_type& k )const
{
   share_type core_fee_required = k.basic_fee;

   share_type unit_fee = k.price_per_k_unit;
   unit_fee *= delta_units;
   unit_fee /= 1000;

   return core_fee_required + unit_fee;
}

void custom_authority_update_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0, "Fee amount can not be negative" );

   FC_ASSERT( account != GRAPHENE_TEMP_ACCOUNT
              && account != GRAPHENE_COMMITTEE_ACCOUNT
              && account != GRAPHENE_WITNESS_ACCOUNT
              && account != GRAPHENE_RELAXED_COMMITTEE_ACCOUNT,
              "Can not create custom authority for special accounts" );
   
   if ( restrictions )
   {
      for( const auto& r : *restrictions )
      {
         operation_type_from_int(*operation_type, operation_type_checker(r));
      }
   }
}

void custom_authority_delete_operation::validate()const
{
}

} } // graphene::chain
