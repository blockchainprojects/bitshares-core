/*
 * Copyright (c) 2019 Blockchain Projects B.V.
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

#include <boost/range/numeric.hpp>
#include <boost/range/adaptor/map.hpp>

#include <fc/optional.hpp>

#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/vote.hpp>

#include <graphene/db/generic_index.hpp>


namespace graphene { namespace chain {

   /**
    * @brief tracks the history of the voting stake for an account
    * @ingroup object
    * @ingroup implementation
    *
    * The calculation of the voting stake, performed in the maintenance interval, results
    * in the creation or, if present, in the update of a voting_statistics_object. 
    *
    * @note By default these objects are not tracked, the voting_stat_plugin must
    * be loaded for these objects to be maintained.
    */
   class voting_statistics_object : public abstract_object<voting_statistics_object>
   {
   public:
      static const uint8_t space_id = protocol_ids;
      static const uint8_t type_id  = voting_statistics_object_type;
      
      voting_statistics_object(){}

      /* the block_num where the maintenance interval was performed */
      uint32_t block_number;
      /* the owner of the stake */
      account_id_type account;
      /* the stake which was generated by this account */
      uint64_t stake = 0;
      /* the proxy for this account */
      account_id_type proxy = GRAPHENE_PROXY_TO_SELF_ACCOUNT;
      /* the accounts which this account was a proxy for with the proxied stakes */
      flat_map< account_id_type, uint64_t > proxy_for;
      /* the vote_id's this account was voting for */
      flat_set<vote_id_type> votes;

      /* returns the total voting stake this account can vote with */
      uint64_t get_total_voting_stake() const
      {
         uint64_t init = has_proxy() ? 0 : stake;
         return boost::accumulate( proxy_for | boost::adaptors::map_values, init );
      }

      inline bool has_proxy() const 
      {
         return GRAPHENE_PROXY_TO_SELF_ACCOUNT != proxy;
      }
   };


   struct by_owner{};

   typedef multi_index_container<
      voting_statistics_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_owner>,
            member< voting_statistics_object, account_id_type, &voting_statistics_object::account >
         >
      >
   > voting_statistics_multi_index_type;

   typedef generic_index<voting_statistics_object, voting_statistics_multi_index_type> voting_statistics_index;

}} // graphene::chain

FC_REFLECT_DERIVED( graphene::chain::voting_statistics_object, (graphene::chain::object),
                    (account)(stake)(proxy)(proxy_for)(votes) )
