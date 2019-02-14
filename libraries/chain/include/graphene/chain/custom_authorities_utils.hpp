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

#include <graphene/chain/protocol/vesting.hpp>
#include <graphene/chain/protocol/worker.hpp>
#include <graphene/chain/protocol/confidential.hpp>
#include <graphene/chain/protocol/account.hpp>
#include <graphene/chain/protocol/assert.hpp>
#include <graphene/chain/protocol/asset_ops.hpp>
#include <graphene/chain/protocol/chain_parameters.hpp>

namespace graphene { namespace chain {
   
typedef fc::static_variant<
   uint8_t,
   uint16_t,
   uint32_t,
   asset_id_type,
   account_id_type,
   balance_id_type,
   proposal_id_type,
   fba_accumulator_id_type,
   limit_order_id_type,
   withdraw_permission_id_type,
   witness_id_type,
   force_settlement_id_type,
   committee_member_id_type,
   public_key_type,
   time_point_sec,
   bool,
   unsigned_int,
   vector<char>,
   string,
   asset,
   price,
   price_feed,
   share_type,
   vesting_policy_initializer,
   worker_initializer,
   extensions_type,
   future_extensions,
   vector<predicate>,
   authority,
   flat_set<account_id_type>,
   flat_set<public_key_type>
   > generic_member;

template <typename T, typename Action>
struct member_visitor
{
   member_visitor(const std::string& member_name, const Action& action, const T& object)
   : m_member_name(member_name)
   , m_action(action)
   , m_object(object)
   {}
   
   typedef void result_type;
   
   template<typename Member, class Class, Member (Class::*member)>
   void operator () ( const char* name ) const
   {
      if (name == m_member_name)
      {
         m_action(m_object.*member);
      }
   }
   
private:
   const std::string m_member_name;
   Action m_action;
   T m_object;
};

template <typename Action>
struct optional_unwrapper_action_decorator
{
   const Action& action;
   
   optional_unwrapper_action_decorator(const Action& an_action)
   : action(an_action)
   {}
  
   template <class T>
   void operator () (const optional<T>& member) const
   {
      if (member)
      {
         action(*member);
      }
   }
   
   template <class T>
   void operator () (const T& member) const
   {
      action(member);
   }
};
   
struct number_to_integer
{
   template <typename T>
   static int64_t convert(const T& number)
   {
      return static_cast<int64_t>(number);
   }
};

struct object_to_integer
{
   template <typename T>
   static int64_t convert(const T& object)
   {
      auto buffer = fc::raw::pack( object );
      return buffer.size();
   }
};
   
struct not_number_to_integer
{
   template <typename T>
   static int64_t convert(const T& number)
   {
      FC_THROW("Can't conver type to integer.");
   }
};

template <typename T>
struct is_fc_reflected_object
{
   static const bool value = fc::reflector<T>::is_defined::value;
};
   
template <typename T>
int64_t to_integer(const T& value)
{
   return boost::mpl::if_<std::is_convertible<T, int64_t>,
                          number_to_integer,
                          typename boost::mpl::if_<is_fc_reflected_object<T>,
                                                   object_to_integer,
                                                   not_number_to_integer
                                                  >::type
                         >::type::convert(value);
}
 
template <>
inline int64_t to_integer<std::string>(const std::string& value)
{
   return static_cast<int64_t>(value.size());
}
   
template <typename T>
inline int64_t to_integer(const vector<T>& value)
{
   return static_cast<int64_t>(value.size());
}

template <typename T>
inline int64_t to_integer(const deque<T>& value)
{
   return static_cast<int64_t>(value.size());
}

template <typename T>
inline int64_t to_integer(const set<T>& value)
{
   return static_cast<int64_t>(value.size());
}

template <typename T>
inline int64_t to_integer(const flat_set<T>& value)
{
   return static_cast<int64_t>(value.size());
}

template <typename K, typename T>
inline int64_t to_integer(const map<K, T>& value)
{
   return static_cast<int64_t>(value.size());
}

template <typename K, typename T>
inline int64_t to_integer(const flat_map<K, T>& value)
{
   return static_cast<int64_t>(value.size());
}

template <typename K, typename T>
inline int64_t to_integer(const unordered_map<K, T>& value)
{
   return static_cast<int64_t>(value.size());
}

template <typename T>
void is_type_supported_by_restriction()
{
   FC_THROW("Type is not supported by restriction. Typename = ${name}", ("name", fc::get_typename<T>::name()));
}
   
template <class T>
bool is_equal(const T& left, const T& right)
{
   FC_THROW("Can't compare types. Type '${type_name}' doesn't support == operator.", ("type_name", fc::get_typename<T>::name()));
}
   
template <typename T>
const T& get(const generic_member& a_variant)
{
   FC_THROW("Can't fetch value. Type '${type_name}' is not supported for now.", ("type_name", fc::get_typename<T>::name()));
}

#define GRAPHENE_RESTRICTION_TYPE(type) \
inline bool is_equal(const type& left, const type& right) {   return left == right; } \
\
template <> \
inline const type& get<type>(const generic_member& a_variant) { return a_variant.get<type>(); } \
\
template <> inline void is_type_supported_by_restriction<type>() {} \

   GRAPHENE_RESTRICTION_TYPE(uint8_t);
   GRAPHENE_RESTRICTION_TYPE(uint16_t);
   GRAPHENE_RESTRICTION_TYPE(uint32_t);
   GRAPHENE_RESTRICTION_TYPE(asset_id_type);
   GRAPHENE_RESTRICTION_TYPE(account_id_type);
   GRAPHENE_RESTRICTION_TYPE(balance_id_type);
   GRAPHENE_RESTRICTION_TYPE(proposal_id_type);
   GRAPHENE_RESTRICTION_TYPE(fba_accumulator_id_type);
   GRAPHENE_RESTRICTION_TYPE(limit_order_id_type);
   GRAPHENE_RESTRICTION_TYPE(withdraw_permission_id_type);
   GRAPHENE_RESTRICTION_TYPE(witness_id_type);
   GRAPHENE_RESTRICTION_TYPE(force_settlement_id_type);
   GRAPHENE_RESTRICTION_TYPE(committee_member_id_type);
   GRAPHENE_RESTRICTION_TYPE(public_key_type);
   GRAPHENE_RESTRICTION_TYPE(time_point_sec);
   GRAPHENE_RESTRICTION_TYPE(bool);
   GRAPHENE_RESTRICTION_TYPE(unsigned_int);
   GRAPHENE_RESTRICTION_TYPE(vector<char>);
   GRAPHENE_RESTRICTION_TYPE(string);
   GRAPHENE_RESTRICTION_TYPE(asset);
   GRAPHENE_RESTRICTION_TYPE(price);
   GRAPHENE_RESTRICTION_TYPE(price_feed);
   GRAPHENE_RESTRICTION_TYPE(share_type);
   GRAPHENE_RESTRICTION_TYPE(vesting_policy_initializer);
   GRAPHENE_RESTRICTION_TYPE(worker_initializer);
   GRAPHENE_RESTRICTION_TYPE(extensions_type);
   GRAPHENE_RESTRICTION_TYPE(future_extensions);
   GRAPHENE_RESTRICTION_TYPE(vector<predicate>);
   GRAPHENE_RESTRICTION_TYPE(authority);
   GRAPHENE_RESTRICTION_TYPE(flat_set<account_id_type>);
   GRAPHENE_RESTRICTION_TYPE(flat_set<public_key_type>);

#undef GRAPHENE_RESTRICTION_TYPE
   
// TODO: should be discussed with bitshares devs and may be fixed in the future
// magic numbers were taken from previous implementation
struct units_calculator_visitor
{
   typedef uint64_t result_type;
   
   template<typename T>
   inline result_type operator()( const T& t )
   {
      return 1;
   }
   
   inline result_type operator()( const fc::sha256& t )
   {
      return 4;
   }
   
   inline result_type operator()( const public_key_type& t )
   {
      return 4;
   }
   
   inline result_type operator()( const string& t )
   {
      return ( t.size() + 7 ) / 8;
   }
   
   template<typename T>
   inline result_type operator()( const vector<T>& list )
   {
      return get_units_for_container(list);
   }
   
   template<typename T>
   inline result_type operator()( const flat_set<T>& list )
   {
      return get_units_for_container(list);
   }
   
private:
   template<typename T>
   result_type get_units_for_container(const T& list)
   {
      result_type result = 0;
      for( const auto& item : list )
      {
         result += (*this)( item );
      }
      return result;
   }
};
   
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
   
} } 
