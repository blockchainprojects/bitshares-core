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

#include <graphene/chain/custom_authority_object.hpp>
#include <fc/reflect/reflect.hpp>
#include <graphene/chain/operation_type_to_id.hpp>

using namespace graphene::chain;

namespace  {
   
   template <typename Operation>
   struct specific_operation_validation_visitor
   {
      Operation op;
      typedef void result_type;
      
      template <class Restriction>
      void operator () ( const Restriction& rest )
      {
         rest.validate(op);
      }
   };
   
   struct operation_validation_visitor
   {
      restriction_v2 rest;
      
      typedef void result_type;
      
      template <class Operation>
      void operator () ( const Operation& op )
      {
         specific_operation_validation_visitor<Operation> visitor;
         visitor.op = op;

         rest.visit(visitor);
      }
   };
   
   void validate_operation_by_restriction( const operation& op, const restriction_v2& rest )
   {
      operation_validation_visitor operation_validator;
      operation_validator.rest = rest;
      
      op.visit(operation_validator);
   }
   
   struct type_id_visitor
   {
      typedef int result_type;
      
      template <class Operation>
      int operator () ( const Operation& )
      {
         return operation_type_id_from_operation_type<Operation>::value;
      }
   };
   
   int get_type_id( const operation& an_operation )
   {
      type_id_visitor visitor;
      return an_operation.visit(visitor);
   }
}

bool custom_authority_object::validate( const operation& op, const time_point_sec now ) const
{
   try
   {
      if (now < valid_from || valid_to < now)
      {
         FC_THROW("Failed to validate the operation because now is not in valid period.");
      }
      
      if (operation_type.value != get_type_id(op))
      {
         FC_THROW("Failed to validate the operation because is has the wrong type.");
      }
      
      for (auto& rest: restrictions)
      {
         validate_operation_by_restriction(op, rest);
      }
      
      return true;
   }
   catch (fc::exception&)
   {
      return false; // validation failed
   }
}
