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
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/committee_member.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace graphene { namespace chain {
   using namespace graphene::db;

   class committee_member_object;
   class committee_proposal_object;

   /**
    *  @brief tracks information about a committee_member account.
    *  @ingroup object
    *
    *  A committee_member is responsible for setting blockchain parameters and has
    *  dynamic multi-sig control over the committee account.  The current set of
    *  active committee_members has control.
    *
    *  committee_members were separated into a separate object to make iterating over
    *  the set of committee_member easy.
    */
   class committee_member_object : public abstract_object<committee_member_object>
   {
      public:
         static const uint8_t space_id = protocol_ids;
         static const uint8_t type_id  = committee_member_object_type;

         account_uid_type    account;
         string              name;
         uint32_t            sequence;
         bool                is_valid = true;
         uint64_t            pledge;
         uint64_t            total_votes = 0;
         string              url;
   };

   struct by_account;
   struct by_valid;
   struct by_votes;
   struct by_pledge;

   typedef multi_index_container<
      committee_member_object,
      indexed_by<
         ordered_unique< tag<by_id>,
            member<object, object_id_type, &object::id>
         >,
         ordered_unique< tag<by_account>,
            composite_key<
               committee_member_object,
               member<committee_member_object, account_uid_type, &committee_member_object::account>,
               member<committee_member_object, uint32_t, &committee_member_object::sequence>
            >
         >,
         ordered_unique< tag<by_valid>,
            composite_key<
               committee_member_object,
               member<committee_member_object, bool, &committee_member_object::is_valid>,
               member<committee_member_object, account_uid_type, &committee_member_object::account>,
               member<committee_member_object, uint32_t, &committee_member_object::sequence>
            >
         >,
         ordered_unique< tag<by_votes>,
            composite_key<
               committee_member_object,
               member<committee_member_object, bool, &committee_member_object::is_valid>,
               member<committee_member_object, uint64_t, &committee_member_object::total_votes>,
               member<committee_member_object, account_uid_type, &committee_member_object::account>,
               member<committee_member_object, uint32_t, &committee_member_object::sequence>
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
               committee_member_object,
               member<committee_member_object, bool, &committee_member_object::is_valid>,
               member<committee_member_object, uint64_t, &committee_member_object::pledge>,
               member<committee_member_object, account_uid_type, &committee_member_object::account>,
               member<committee_member_object, uint32_t, &committee_member_object::sequence>
            >,
            composite_key_compare<
               std::less< bool >,
               std::greater< uint64_t >,
               std::less< account_uid_type >,
               std::less< uint32_t >
            >
         >
      >
   > committee_member_multi_index_type;

   /**
    * @ingroup object_index
    */
   typedef generic_index<committee_member_object, committee_member_multi_index_type> committee_member_index;

   /**
    * @brief This class represents a committee member voting on the object graph
    * @ingroup object
    * @ingroup protocol
    */
   class committee_member_vote_object : public graphene::db::abstract_object<committee_member_vote_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_committee_member_vote_object_type;

         account_uid_type    voter_uid = 0;
         uint32_t            voter_sequence;
         account_uid_type    committee_member_uid = 0;
         uint32_t            committee_member_sequence;
   };

   struct by_voter_seq;
   struct by_committee_member_seq;

   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      committee_member_vote_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_voter_seq>,
            composite_key<
               committee_member_vote_object,
               member< committee_member_vote_object, account_uid_type, &committee_member_vote_object::voter_uid>,
               member< committee_member_vote_object, uint32_t, &committee_member_vote_object::voter_sequence>,
               member< committee_member_vote_object, account_uid_type, &committee_member_vote_object::committee_member_uid>,
               member< committee_member_vote_object, uint32_t, &committee_member_vote_object::committee_member_sequence>
            >
         >,
         ordered_unique< tag<by_committee_member_seq>,
            composite_key<
               committee_member_vote_object,
               member< committee_member_vote_object, account_uid_type, &committee_member_vote_object::committee_member_uid>,
               member< committee_member_vote_object, uint32_t, &committee_member_vote_object::committee_member_sequence>,
               member< committee_member_vote_object, account_uid_type, &committee_member_vote_object::voter_uid>,
               member< committee_member_vote_object, uint32_t, &committee_member_vote_object::voter_sequence>
            >
         >
      >
   > committee_member_vote_multi_index_type;

   /**
    * @ingroup object_index
    */
   typedef generic_index<committee_member_vote_object, committee_member_vote_multi_index_type> committee_member_vote_index;

   /**
    * @brief This class represents a committee proposal on the object graph
    * @ingroup object
    * @ingroup protocol
    */
   class committee_proposal_object : public graphene::db::abstract_object<committee_proposal_object>
   {
      public:
         static const uint8_t space_id = protocol_ids;
         static const uint8_t type_id  = committee_proposal_object_type;

         committee_proposal_number_type                  proposal_number = 0;
         account_uid_type                                proposer;
         vector<committee_proposal_item_type>            items;
         uint32_t                                        voting_closing_block_num;
         uint32_t                                        execution_block_num;
         uint32_t                                        expiration_block_num;
         flat_map<account_uid_type, voting_opinion_type> opinions;
         uint16_t                                        approve_threshold;
         bool                                            is_approved = false;

         uint16_t          get_approve_threshold(const uint8_t enable_hard_fork_type)const;
         void              update_approve_threshold(const uint8_t enable_hard_fork_type);
   };

   struct by_number;
   struct by_proposer;
   struct by_closing_block;
   struct by_approved_closing_block;
   struct by_approved_execution_block;

   typedef multi_index_container<
      committee_proposal_object,
      indexed_by<
         ordered_unique< tag<by_id>,
            member<object, object_id_type, &object::id>
         >,
         ordered_unique< tag<by_number>,
            member<committee_proposal_object, committee_proposal_number_type, &committee_proposal_object::proposal_number>
         >,
         ordered_unique< tag<by_approved_closing_block>,
            composite_key<
               committee_proposal_object,
               member<committee_proposal_object, bool, &committee_proposal_object::is_approved>,
               member<committee_proposal_object, uint32_t, &committee_proposal_object::voting_closing_block_num>,
               member<committee_proposal_object, committee_proposal_number_type, &committee_proposal_object::proposal_number>
            >
         >,
         ordered_unique< tag<by_approved_execution_block>,
            composite_key<
               committee_proposal_object,
               member<committee_proposal_object, bool, &committee_proposal_object::is_approved>,
               member<committee_proposal_object, uint32_t, &committee_proposal_object::execution_block_num>,
               member<committee_proposal_object, committee_proposal_number_type, &committee_proposal_object::proposal_number>
            >
         >,
         ordered_unique< tag<by_proposer>, // for API
            composite_key<
               committee_proposal_object,
               member<committee_proposal_object, account_uid_type, &committee_proposal_object::proposer>,
               member<committee_proposal_object, committee_proposal_number_type, &committee_proposal_object::proposal_number>
            >
         >,
         ordered_unique< tag<by_closing_block>, // for API
            composite_key<
               committee_proposal_object,
               member<committee_proposal_object, uint32_t, &committee_proposal_object::voting_closing_block_num>,
               member<committee_proposal_object, committee_proposal_number_type, &committee_proposal_object::proposal_number>
            >
         >
      >
   > committee_proposal_multi_index_type;

   /**
    * @ingroup object_index
    */
   typedef generic_index<committee_proposal_object, committee_proposal_multi_index_type> committee_proposal_index;

} } // graphene::chain

FC_REFLECT_DERIVED( graphene::chain::committee_member_object, (graphene::db::object),
                    (account)
                    (name)
                    (sequence)
                    (is_valid)
                    (pledge)
                    (total_votes)
                    (url)
                  )

FC_REFLECT_DERIVED( graphene::chain::committee_member_vote_object, (graphene::db::object),
                    (voter_uid)
                    (voter_sequence)
                    (committee_member_uid)
                    (committee_member_sequence)
                  )

FC_REFLECT_DERIVED( graphene::chain::committee_proposal_object, (graphene::db::object),
                    (proposal_number)
                    (proposer)
                    (items)
                    (voting_closing_block_num)
                    (execution_block_num)
                    (expiration_block_num)
                    (opinions)
                    (approve_threshold)
                    (is_approved)
                  )
