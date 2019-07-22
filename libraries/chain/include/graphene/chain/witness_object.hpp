/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
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
#include <graphene/chain/protocol/asset.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace graphene { namespace chain {
   using namespace graphene::db;

   class witness_object;

   /**
    * @brief This class represents a witness on the object graph
    * @ingroup object
    * @ingroup protocol
    */
   class witness_object : public abstract_object<witness_object>
   {
      public:
         static const uint8_t space_id = protocol_ids;
         static const uint8_t type_id = witness_object_type;

         account_uid_type    account;
         string              name;
         uint32_t            sequence;
         bool                is_valid = true;

         public_key_type     signing_key;

         uint64_t            pledge;
         fc::time_point_sec  pledge_last_update;
         uint64_t            average_pledge = 0;
         fc::time_point_sec  average_pledge_last_update;
         uint32_t            average_pledge_next_update_block;

         uint64_t            total_votes = 0;

         fc::uint128_t       by_pledge_position;
         fc::uint128_t       by_pledge_position_last_update;
         fc::uint128_t       by_pledge_scheduled_time = fc::uint128_t::max_value();

         fc::uint128_t       by_vote_position;
         fc::uint128_t       by_vote_position_last_update;
         fc::uint128_t       by_vote_scheduled_time = fc::uint128_t::max_value();

         uint32_t            last_confirmed_block_num = 0;

         uint64_t            last_aslot = 0;
         uint64_t            total_produced = 0;
         uint64_t            total_missed = 0;
         string              url;

         ///account pledge asset to witness switch
         bool                can_pledge = false;
         ///part of witness pay as a bonus that divided to pledge account 
         uint32_t            bonus_rate = 0;
         ///account pledge asset to witness, total
         uint64_t            total_mining_pledge = 0;
         ///map<head block num, bonus_per_pledge>
         map<uint32_t, share_type> bonus_per_pledge;
         share_type          unhandled_bonus;
         share_type          need_distribute_bonus;
         share_type          already_distribute_bonus;
         uint32_t            last_update_bonus_block_num = 0;

         uint32_t get_bonus_block_num()const {
            if (total_mining_pledge > 0 && (!bonus_per_pledge.empty() || unhandled_bonus > 0))
               return last_update_bonus_block_num + 10000;
            else
               return -1;
            }

         share_type accumulate_bonus_per_pledge(uint32_t start_block_num)const {
            auto itr = bonus_per_pledge.lower_bound(start_block_num);
            share_type result= std::accumulate(itr, bonus_per_pledge.end(), 0, 
               [](uint64_t bonus, std::pair<uint32_t, share_type> p) {
               return bonus + p.second.value; 
            });
            
            if (unhandled_bonus > 0)
            {
               result = result + ((fc::uint128_t)unhandled_bonus.value* GRAPHENE_PLEDGE_BONUS_PRECISION
                  / total_mining_pledge).to_uint64();
            }

            return result;
         }
   };

   struct by_account;
   struct by_pledge_next_update;
   struct by_pledge_schedule;
   struct by_vote_schedule;
   struct by_valid;
   struct by_pledge;
   struct by_votes;
   struct by_pledge_mining_bonus;

   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      witness_object,
      indexed_by<
         ordered_unique< tag<by_id>,
            member<object, object_id_type, &object::id>
         >,
         ordered_unique< tag<by_account>,
            composite_key<
               witness_object,
               member<witness_object, account_uid_type, &witness_object::account>,
               member<witness_object, uint32_t, &witness_object::sequence>
            >
         >,
         ordered_unique< tag<by_pledge_next_update>,
            composite_key<
               witness_object,
               member<witness_object, uint32_t, &witness_object::average_pledge_next_update_block>,
               member<witness_object, account_uid_type, &witness_object::account>,
               member<witness_object, uint32_t, &witness_object::sequence>
            >
         >,
         ordered_unique< tag<by_pledge_schedule>,
            composite_key<
               witness_object,
               member<witness_object, bool, &witness_object::is_valid>,
               member<witness_object, fc::uint128_t, &witness_object::by_pledge_scheduled_time>,
               member<witness_object, account_uid_type, &witness_object::account>,
               member<witness_object, uint32_t, &witness_object::sequence>
            >
         >,
         ordered_unique< tag<by_vote_schedule>,
            composite_key<
               witness_object,
               member<witness_object, bool, &witness_object::is_valid>,
               member<witness_object, fc::uint128_t, &witness_object::by_vote_scheduled_time>,
               member<witness_object, account_uid_type, &witness_object::account>,
               member<witness_object, uint32_t, &witness_object::sequence>
            >
         >,
         ordered_unique< tag<by_valid>,
            composite_key<
               witness_object,
               member<witness_object, bool, &witness_object::is_valid>,
               member<witness_object, account_uid_type, &witness_object::account>,
               member<witness_object, uint32_t, &witness_object::sequence>
            >
         >,
         ordered_unique< tag<by_votes>, // for API
            composite_key<
               witness_object,
               member<witness_object, bool, &witness_object::is_valid>,
               member<witness_object, uint64_t, &witness_object::total_votes>,
               member<witness_object, account_uid_type, &witness_object::account>,
               member<witness_object, uint32_t, &witness_object::sequence>
            >,
            composite_key_compare<
               std::less< bool >,
               std::greater< uint64_t >,
               std::less< account_uid_type >,
               std::less< uint32_t >
            >
         >,
         ordered_unique< tag<by_pledge>, // for API
            composite_key<
               witness_object,
               member<witness_object, bool, &witness_object::is_valid>,
               member<witness_object, uint64_t, &witness_object::pledge>,
               member<witness_object, account_uid_type, &witness_object::account>,
               member<witness_object, uint32_t, &witness_object::sequence>
            >,
            composite_key_compare<
               std::less< bool >,
               std::greater< uint64_t >,
               std::less< account_uid_type >,
               std::less< uint32_t >
            >
         >,
         ordered_non_unique< tag<by_pledge_mining_bonus>, 
            const_mem_fun<witness_object, uint32_t, &witness_object::get_bonus_block_num  >,
            std::greater<uint32_t>
         >
      >
   > witness_multi_index_type;

   /**
    * @ingroup object_index
    */
   typedef generic_index<witness_object, witness_multi_index_type> witness_index;


   /**
    * @brief This class represents a witness voting on the object graph
    * @ingroup object
    * @ingroup protocol
    */
   class witness_vote_object : public graphene::db::abstract_object<witness_vote_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_witness_vote_object_type;

         account_uid_type    voter_uid = 0;
         uint32_t            voter_sequence;
         account_uid_type    witness_uid = 0;
         uint32_t            witness_sequence;
   };

   struct by_voter_seq;
   struct by_witness_seq;

   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      witness_vote_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_voter_seq>,
            composite_key<
               witness_vote_object,
               member< witness_vote_object, account_uid_type, &witness_vote_object::voter_uid>,
               member< witness_vote_object, uint32_t, &witness_vote_object::voter_sequence>,
               member< witness_vote_object, account_uid_type, &witness_vote_object::witness_uid>,
               member< witness_vote_object, uint32_t, &witness_vote_object::witness_sequence>
            >
         >,
         ordered_unique< tag<by_witness_seq>,
            composite_key<
               witness_vote_object,
               member< witness_vote_object, account_uid_type, &witness_vote_object::witness_uid>,
               member< witness_vote_object, uint32_t, &witness_vote_object::witness_sequence>,
               member< witness_vote_object, account_uid_type, &witness_vote_object::voter_uid>,
               member< witness_vote_object, uint32_t, &witness_vote_object::voter_sequence>
            >
         >
      >
   > witness_vote_multi_index_type;

   /**
    * @ingroup object_index
    */
   typedef generic_index<witness_vote_object, witness_vote_multi_index_type> witness_vote_index;

} } // graphene::chain

FC_REFLECT_DERIVED( graphene::chain::witness_object, (graphene::db::object),
                    (account)
                    (name)
                    (sequence)
                    (is_valid)
                    (signing_key)
                    (pledge)
                    (pledge_last_update)
                    (average_pledge)
                    (average_pledge_last_update)
                    (average_pledge_next_update_block)
                    (total_votes)
                    (by_pledge_position)
                    (by_pledge_position_last_update)
                    (by_pledge_scheduled_time)
                    (by_vote_position)
                    (by_vote_position_last_update)
                    (by_vote_scheduled_time)
                    (last_confirmed_block_num)
                    (last_aslot)
                    (total_produced)
                    (total_missed)
                    (url)
                    (can_pledge)
                    (bonus_rate)
                    (total_mining_pledge)
                    (bonus_per_pledge)
                    (unhandled_bonus)
                    (need_distribute_bonus)
                    (already_distribute_bonus)
                    (last_update_bonus_block_num)
                  )

FC_REFLECT_DERIVED( graphene::chain::witness_vote_object, (graphene::db::object),
                    (voter_uid)
                    (voter_sequence)
                    (witness_uid)
                    (witness_sequence)
                  )