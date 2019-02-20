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
#pragma once

#include <graphene/chain/protocol/operations.hpp>

#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>

namespace graphene { namespace chain {

template <typename OperationType>
struct int_from_operation_type
{
   static const int value = -1;
};

#define GRAPHENE_GENERATE_OP_TYPE_TO_ID_MAPPER(r, data, i, elem) \
template <> \
struct int_from_operation_type<elem> \
{ \
   static const int value = i; \
}; \


BOOST_PP_SEQ_FOR_EACH_I( GRAPHENE_GENERATE_OP_TYPE_TO_ID_MAPPER, , BOOST_PP_VARIADIC_TO_SEQ( GRAPHENE_OPERATIONS_VARIADIC ) )

#undef GRAPHENE_GENERATE_OP_TYPE_TO_ID_MAPPER

template <typename Action>
void operation_type_from_int(const int num, const Action& action)
{
#define GRAPHENE_GENERATE_OP_ID_TO_TYPE_MAPPER(r, data, i, elem) \
      case i: \
         action.template operator()<elem>(); \
         break;
   
   switch (num)
   {
         BOOST_PP_SEQ_FOR_EACH_I( GRAPHENE_GENERATE_OP_ID_TO_TYPE_MAPPER, , BOOST_PP_VARIADIC_TO_SEQ( GRAPHENE_OPERATIONS_VARIADIC ) )
   }
   
#undef GRAPHENE_GENERATE_OP_ID_TO_TYPE_MAPPER
   
}

} } 
