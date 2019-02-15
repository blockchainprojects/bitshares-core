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
#include <graphene/chain/custom_authority_evaluator.hpp>
#include <graphene/chain/custom_authority_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>

namespace graphene { namespace chain {

void_result custom_authority_create_evaluator::do_evaluate(const custom_authority_create_operation& op)
{ try {
   FC_ASSERT(db().head_block_time() > HARDFORK_CORE_1285_TIME,
             "custom_authority_create_operation should not be executed before HARDFORK_CORE_1285_TIME");
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type custom_authority_create_evaluator::do_apply(const custom_authority_create_operation& op)
{ try {
   database& d = db();

   const auto& new_object = d.create<custom_authority_object>( [&op]( custom_authority_object& obj ){
      obj.account        = op.account;
      obj.auth           = op.auth;
      obj.enabled        = op.enabled;
      obj.valid_from     = op.valid_from;
      obj.valid_to       = op.valid_to;
      obj.operation_type = op.operation_type;
      obj.restrictions   = op.restrictions;
   });

   return new_object.id;

} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result custom_authority_update_evaluator::do_evaluate(const custom_authority_update_operation& op)
{ try {
   FC_ASSERT(db().head_block_time() > HARDFORK_CORE_1285_TIME,
             "custom_authority_update_operation should not be executed before HARDFORK_CORE_1285_TIME");
   
   //check if object exists
   db().get<custom_authority_object>(op.custom_authority_to_update);
   
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result custom_authority_update_evaluator::do_apply(const custom_authority_update_operation& op)
{ try {
   database& d = db();
   
   d.modify( d.get<custom_authority_object>(op.custom_authority_to_update), [&]( custom_authority_object& obj ){
      
      if (op.auth)
      {
         obj.auth = *op.auth;
      }
      
      if (op.enabled)
      {
         obj.enabled = *op.enabled;
      }
      
      if (op.valid_to)
      {
         obj.valid_to = *op.valid_to;
      }
      
      if (op.operation_type)
      {
         obj.operation_type = *op.operation_type;
      }
      
      if (op.restrictions)
      {
         obj.restrictions = *op.restrictions;
      }
   });
   
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result custom_authority_delete_evaluator::do_evaluate(const custom_authority_delete_operation& op)
{ try {
   FC_ASSERT(db().head_block_time() > HARDFORK_CORE_1285_TIME,
             "custom_authority_delete_operation should not be executed before HARDFORK_CORE_1285_TIME");
   
   //check if object exists
   db().get<custom_authority_object>(op.custom_authority_to_delete);
   
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result custom_authority_delete_evaluator::do_apply(const custom_authority_delete_operation& op)
{ try {
   database& d = db();

   d.remove(d.get<custom_authority_object>(op.custom_authority_to_delete));

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

} } // graphene::chain
