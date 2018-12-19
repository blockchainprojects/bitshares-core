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
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/custom_authorities_utils.hpp>

namespace graphene { namespace chain {
   
struct is_type_supported_by_base_restriction
{
   template <class T>
   void operator () (const T& member) const
   {
      is_type_supported_by_restriction<T>();
   }
};

struct is_type_supported_by_base_list_restriction
{
   template <class T>
   void operator () (const T&) const
   {
      FC_THROW("List restriction argument is not a list.");
   }
   
   template <class T>
   void operator () (const flat_set<T>&) const
   {
      is_type_supported_by_restriction<T>();
   }
};
   
template <typename Action>
struct base_restriction
{
   generic_member value;
   std::string argument;
   
   template <typename Operation>
   void validate( const Operation& op ) const
   {
      Action action(value);
      member_visitor<Operation, optional_unwrapper_action_decorator<Action>> visitor(argument, optional_unwrapper_action_decorator<Action>(action), op);
      fc::reflector<Operation>::visit(visitor);
   }
   
   template <typename Operation>
   void validate() const
   {
      member_visitor<Operation, is_type_supported_by_base_restriction> visitor(argument, is_type_supported_by_base_restriction(), Operation());
      fc::reflector<Operation>::visit(visitor);
   }
};

template <typename Action>
struct base_list_restriction
{
   std::vector<generic_member> values;
   std::string argument;
   
   template <typename Operation>
   void validate( const Operation& op ) const
   {
      Action action(values);
      member_visitor<Operation, optional_unwrapper_action_decorator<Action>> visitor(argument, optional_unwrapper_action_decorator<Action>(action), op);
      fc::reflector<Operation>::visit(visitor);
   }
   
   template <typename Operation>
   void validate() const
   {
      member_visitor<Operation, is_type_supported_by_base_list_restriction> visitor(argument, is_type_supported_by_base_list_restriction(), Operation());
      fc::reflector<Operation>::visit(visitor);
   }
};
    
template <typename Action>
struct base_comparision_restriction
{
   uint64_t value;
   std::string argument;
   
   template <typename Operation>
   void validate( const Operation& op ) const
   {
      Action action(value);
      member_visitor<Operation, optional_unwrapper_action_decorator<Action>> visitor(argument, optional_unwrapper_action_decorator<Action>(action), op);
      fc::reflector<Operation>::visit(visitor);
   }
   
   template <typename Operation>
   void validate() const // should support all arguments of all operations
   {}
};

class equal
{
public:
   equal(const generic_member& value)
   : m_value(value)
   {}
   
   template <class T>
   void operator () (const T& member) const
   {
      FC_ASSERT(is_equal(get<T>(m_value), member), "Restriction value is not equal to member argument.");
   }
   
private:
   generic_member m_value;
};

class not_equal
{
public:
   not_equal(const generic_member& value)
   : m_value(value)
   {}
   
   template <class T>
   void operator () (const T& member) const
   {
      FC_ASSERT(!is_equal(get<T>(m_value), member), "Restriction value is equal to member argument.");
   }
   
private:
   generic_member m_value;
};
    
class less
{
public:
    less(const int64_t value)
    : m_value(value)
    {}
    
    template <class T>
    void operator () (const T& member) const
    {
        FC_ASSERT(to_integer(member) < m_value, "Argument is not less than value.");
    }
    
private:
    int64_t m_value;
};

class less_or_equal
{
public:
    less_or_equal(const int64_t value)
    : m_value(value)
    {}
    
    template <class T>
    void operator () (const T& member) const
    {
        FC_ASSERT(to_integer(member) <= m_value, "Argument is not less or equal than value.");
    }
    
private:
    int64_t m_value;
};

class greater
{
public:
    greater(const int64_t value)
    : m_value(value)
    {}
    
    template <class T>
    void operator () (const T& member) const
    {
        FC_ASSERT(to_integer(member) > m_value, "Argument is not greater than value.");
    }
    
private:
    int64_t m_value;
};
    
class greater_or_equal
{
public:
    greater_or_equal(const int64_t value)
    : m_value(value)
    {}
    
    template <class T>
    void operator () (const T& member) const
    {
        FC_ASSERT(to_integer(member) >= m_value, "Argument is not greater or equal than value.");
    }
    
private:
    int64_t m_value;
};
    
class any_of
{
public:
   any_of(const std::vector<generic_member>& values)
   : m_values(values)
   {}
   
   template <class T>
   void operator () (const T& member) const
   {
      for (const generic_member& value: m_values)
      {
         if (is_equal(get<T>(value), member))
         {
            return;
         }
      }
      
      FC_THROW("Argument was not present in the list.");
   }
   
private:
   std::vector<generic_member> m_values;
};

class none_of
{
public:
   none_of(const std::vector<generic_member>& values)
   : m_values(values)
   {}
   
   template <class T>
   void operator () (const T& member) const
   {
      for (const generic_member& value: m_values)
      {
         if (is_equal(get<T>(value), member))
         {
            FC_THROW("Operation member is present in the list.");
         }
      }
   }
   
private:
   std::vector<generic_member> m_values;
};

class contains_all
{
public:
   contains_all(const std::vector<generic_member>& values)
   : m_values(values)
   {}
   
   template <class T>
   void operator () (const T&) const
   {
      FC_THROW("Not list type come.");
   }
   
   template <class T>
   void operator () (const flat_set<T>& list) const
   {
      for (const generic_member& value: m_values)
      {
         bool contains = false;
         for (const auto& item: list)
         {
            contains |= (item == value.get<T>());
         }
         
         FC_ASSERT(contains, "Conatains all restriction value is not contained by member argument.");
      }
   }
   
private:
   std::vector<generic_member> m_values;
};

class contains_none
{
public:
   contains_none(const std::vector<generic_member>& values)
   : m_values(values)
   {}
   
   template <class T>
   void operator () (const T&) const
   {
      FC_THROW("Not list type come.");
   }
   
   template <class T>
   void operator () (const flat_set<T>& list) const
   {
      for (const generic_member& value: m_values)
      {
         for (const auto& item: list)
         {
            if (is_equal(item, get<T>(value)))
            {
               FC_THROW("Should not contain any of same items.");
            }
         }
      }
   }
   
private:
   std::vector<generic_member> m_values;
};

typedef base_restriction<equal>                        eq_restriction;
typedef base_restriction<not_equal>                    neq_restriction;
typedef base_comparision_restriction<less>             lt_restriction;
typedef base_comparision_restriction<less_or_equal>    le_restriction;
typedef base_comparision_restriction<greater>          gt_restriction;
typedef base_comparision_restriction<greater_or_equal> ge_restriction;
typedef base_list_restriction<any_of>               any_restriction;
typedef base_list_restriction<none_of>              none_restriction;
typedef base_list_restriction<contains_all>         contains_all_restriction;
typedef base_list_restriction<contains_none>        contains_none_restriction;

struct restriction_holder;

struct attribute_assert
{
   string argument;
   vector<restriction_holder> restrictions;
   
   template <typename Operation>
   void validate( const Operation& op ) const
   {
     
   }
   
   template <typename Operation>
   void validate() const
   {
     
   }
};
    
   typedef fc::static_variant</*lt_restriction,*/
                           eq_restriction,
                           neq_restriction,
                           any_restriction,
                           none_restriction,
                           contains_all_restriction,
                           contains_none_restriction,
                           attribute_assert> restriction_v2;
   
struct restriction_holder
{
   restriction_holder() = default;
   
   restriction_holder(const restriction_v2& a_rest)
   : rest(a_rest)
   {}
   
   restriction_v2 rest;
};

} }

FC_REFLECT( graphene::chain::lt_restriction,
           (value)
           (argument)
           )

FC_REFLECT( graphene::chain::le_restriction,
           (value)
           (argument)
           )

FC_REFLECT( graphene::chain::gt_restriction,
           (value)
           (argument)
           )

FC_REFLECT( graphene::chain::ge_restriction,
           (value)
           (argument)
           )

FC_REFLECT( graphene::chain::eq_restriction,
           (value)
           (argument)
           )

FC_REFLECT( graphene::chain::neq_restriction,
           (value)
           (argument)
           )

FC_REFLECT( graphene::chain::any_restriction,
           (values)
           (argument)
           )

FC_REFLECT( graphene::chain::none_restriction,
           (values)
           (argument)
           )

FC_REFLECT( graphene::chain::contains_all_restriction,
           (values)
           (argument)
           )

FC_REFLECT( graphene::chain::contains_none_restriction,
           (values)
           (argument)
           )

FC_REFLECT( graphene::chain::restriction_holder,
           (rest)
           )

FC_REFLECT( graphene::chain::attribute_assert,
           (argument)
           (restrictions)
           )
