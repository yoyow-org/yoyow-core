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
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/chain_parameters.hpp>
#include <fc/smart_ref_fwd.hpp>

namespace graphene { namespace chain { 

   /**
    * @brief Create a committee_member object, as a bid to hold a committee_member seat on the network.
    * @ingroup operations
    *
    * Accounts which wish to become committee_members may use this operation to create a committee_member object which stakeholders may
    * vote on to approve its position as a committee_member.
    */
   struct committee_member_create_operation : public base_operation
   {
      struct fee_parameters_type
      {
         uint64_t fee              = 100 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee     = 100 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint16_t min_rf_percent   = 10000;
         extensions_type   extensions;
      };

      fee_type          fee;
      /// The account which owns the committee_member. This account pays the fee for this operation.
      account_uid_type  account;
      asset             pledge;
      string            url;
      extensions_type   extensions;

      account_uid_type  fee_payer_uid()const { return account; }
      void              validate()const;
      //share_type      calculate_fee(const fee_parameters_type& k)const;
      void get_required_active_uid_authorities( flat_set<account_uid_type>& a )const
      {
         // need active authority
         a.insert( account );
      }
   };

   /**
    * @brief Update a committee_member object.
    * @ingroup operations
    */
   struct committee_member_update_operation : public base_operation
   {
      struct fee_parameters_type
      {
         uint64_t fee              = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee     = 0;
         uint16_t min_rf_percent   = 0;
         extensions_type   extensions;
      };

      fee_type          fee;
      /// The account which owns the committee member. This account pays the fee for this operation.
      account_uid_type  account;

      /// The new pledge
      optional< asset           >  new_pledge;
      /// The new URL.
      optional< string          >  new_url;

      extensions_type   extensions;

      account_uid_type  fee_payer_uid()const { return account; }
      void              validate()const;
      //share_type      calculate_fee(const fee_parameters_type& k)const;
      void get_required_active_uid_authorities( flat_set<account_uid_type>& a )const
      {
         // need active authority
         a.insert( account );
      }
   };

   /**
    * @brief Change or refresh committee member voting status.
    * @ingroup operations
    */
   struct committee_member_vote_update_operation : public base_operation
   {
      struct fee_parameters_type
      {
         uint64_t fee               = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee      = 0;
         uint16_t min_rf_percent    = 0;
         extensions_type   extensions;
      };

      fee_type                   fee;
      /// The account which vote for committee members. This account pays the fee for this operation.
      account_uid_type           voter;
      flat_set<account_uid_type> committee_members_to_add;
      flat_set<account_uid_type> committee_members_to_remove;

      extensions_type   extensions;

      account_uid_type  fee_payer_uid()const { return voter; }
      void              validate()const;
      //share_type      calculate_fee(const fee_parameters_type& k)const;
      void get_required_active_uid_authorities( flat_set<account_uid_type>& a )const
      {
         // need active authority
         a.insert( voter );
      }
   };

   /**
    * @ingroup operations
    *
    * Committee update individial account priviledges.
    */
   struct committee_update_account_priviledge_item_type
   {
      struct account_priviledge_update_options
      {
         optional<bool>             can_vote;
         optional<bool>             is_admin;
         optional<bool>             is_registrar;
         optional<account_uid_type> takeover_registrar;
      };
      account_uid_type                              account;
      extension<account_priviledge_update_options>  new_priviledges;

      void validate()const;
   };

   /**
    * @ingroup operations
    *
    * Committee update fee schedule.
    */
   typedef smart_ref<fee_schedule> committee_update_fee_schedule_item_type;

   /**
    * @ingroup operations
    *
    * The global chain parameters that the committee can update.
    */
   struct committee_updatable_parameters
   {
      optional< uint32_t              >  maximum_transaction_size            ;
      optional< uint32_t              >  maximum_block_size                  ;
      optional< uint32_t              >  maximum_time_until_expiration       ;
      optional< uint16_t              >  maximum_authority_membership        ;
      optional< uint8_t               >  max_authority_depth                 ;
      optional< uint64_t              >  csaf_rate                           ;
      optional< share_type            >  max_csaf_per_account                ;
      optional< uint64_t              >  csaf_accumulate_window              ;
      optional< uint64_t              >  min_witness_pledge                  ;
      optional< uint64_t              >  max_witness_pledge_seconds          ;
      optional< uint32_t              >  witness_avg_pledge_update_interval  ;
      optional< uint32_t              >  witness_pledge_release_delay        ;
      optional< uint64_t              >  min_governance_voting_balance       ;
      // Although theoretically this parameter can be updated, but we didn't write related code
      //    to deal with the update, so don't update it so far.
      //optional< uint8_t               >  max_governance_voting_proxy_level   ;
      optional< uint32_t              >  governance_voting_expiration_blocks ;
      optional< uint32_t              >  governance_votes_update_interval    ;
      optional< uint64_t              >  max_governance_votes_seconds        ;
      optional< uint16_t              >  max_witnesses_voted_per_account     ;
      optional< uint32_t              >  max_witness_inactive_blocks         ;
      optional< share_type            >  by_vote_top_witness_pay_per_block   ;
      optional< share_type            >  by_vote_rest_witness_pay_per_block  ;
      optional< share_type            >  by_pledge_witness_pay_per_block     ;
      optional< uint16_t              >  by_vote_top_witness_count           ;
      optional< uint16_t              >  by_vote_rest_witness_count          ;
      optional< uint16_t              >  by_pledge_witness_count             ;
      optional< uint32_t              >  budget_adjust_interval              ;
      optional< uint16_t              >  budget_adjust_target                ;
      // Don't update this parameter so far
      //optional< uint8_t               >  committee_size                      ;
      // Don't update this parameter so far
      //optional< uint32_t              >  committee_update_interval           ;
      optional< uint64_t              >  min_committee_member_pledge         ;
      optional< uint32_t              >  committee_member_pledge_release_delay   ;
      // Don't update this parameter so far
      //optional< uint16_t              >  max_committee_members_voted_per_account ;
      optional< uint32_t              > witness_report_prosecution_period        ;
      optional< bool                  > witness_report_allow_pre_last_block      ;
      optional< share_type            > witness_report_pledge_deduction_amount   ;

      optional< uint64_t            > platform_min_pledge             ;
      optional< uint32_t            > platform_pledge_release_delay   ;
      optional< uint8_t             > platform_max_vote_per_account   ;

      void validate()const;
   };

   /**
    * @ingroup operations
    *
    * Committee update global chain parameters.
    */
   typedef extension<committee_updatable_parameters> committee_update_global_parameter_item_type;

   /**
    * @ingroup operations
    *
    * Defines the set of valid committee proposal items as a tagged union type.
    */
   typedef fc::static_variant <
            committee_update_account_priviledge_item_type,
            committee_update_fee_schedule_item_type,
            committee_update_global_parameter_item_type
         > committee_proposal_item_type;

   /**
    * Opinion on a proposal
    */
   enum voting_opinion_type
   {
      opinion_against = -1,
      opinion_neutral = 0,
      opinion_for = 1
   };

   /**
    * @brief A committee member propose a proposal
    * @ingroup operations
    */
   struct committee_proposal_create_operation : public base_operation
   {
      struct fee_parameters_type
      {
         uint64_t basic_fee        = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t price_per_item   = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee     = 0;
         uint16_t min_rf_percent   = 0;
         extensions_type   extensions;
      };

      fee_type                             fee;
      /// The account which owns the committee member. This account pays the fee for this operation.
      account_uid_type                     proposer;
      /// The proposed items
      vector<committee_proposal_item_type> items;
      /// When will voting for the proposal be closed
      uint32_t                             voting_closing_block_num;
      /// When will the proposal be executed if approved
      uint32_t                             execution_block_num;
      /// When will the proposal be tried to executed again if failed on first execution, will be ignored if failed again.
      uint32_t                             expiration_block_num;
      /// Whether the proposer agree with the proposal
      optional<voting_opinion_type>        proposer_opinion;

      extensions_type   extensions;

      account_uid_type  fee_payer_uid()const { return proposer; }
      void              validate()const;
      share_type        calculate_fee(const fee_parameters_type& k)const;
      void get_required_active_uid_authorities( flat_set<account_uid_type>& a )const
      {
         // need active authority
         a.insert( proposer );
      }
   };

   /**
    * @brief A committee member update a proposal
    * @ingroup operations
    */
   struct committee_proposal_update_operation : public base_operation
   {
      struct fee_parameters_type
      {
         uint64_t fee              = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee     = 0;
         uint16_t min_rf_percent   = 0;
         extensions_type   extensions;
      };

      fee_type                             fee;
      /// The account which owns the committee member. This account pays the fee for this operation.
      account_uid_type                     account = 0;
      /// The ID of proposal
      committee_proposal_number_type       proposal_number = 0;
      /// Whether the committee member agree with the proposal
      voting_opinion_type                  opinion = opinion_neutral;

      extensions_type   extensions;

      account_uid_type  fee_payer_uid()const { return account; }
      void              validate()const;
      //share_type        calculate_fee(const fee_parameters_type& k)const;
      void get_required_active_uid_authorities( flat_set<account_uid_type>& a )const
      {
         // need active authority
         a.insert( account );
      }
   };

   /**
    * @brief Used by committee_members to update the global parameters of the blockchain.
    * @ingroup operations
    *
    * This operation allows the committee_members to update the global parameters on the blockchain. These control various
    * tunable aspects of the chain, including block and maintenance intervals, maximum data sizes, the fees charged by
    * the network, etc.
    *
    * This operation may only be used in a proposed transaction, and a proposed transaction which contains this
    * operation must have a review period specified in the current global parameters before it may be accepted.
    */
   struct committee_member_update_global_parameters_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION; };

      asset             fee;
      chain_parameters  new_parameters;

      account_id_type fee_payer()const { return account_id_type(); }
      void            validate()const;
   };

   /// TODO: committee_member_resign_operation : public base_operation

} } // graphene::chain

FC_REFLECT( graphene::chain::committee_member_create_operation::fee_parameters_type,
            (fee)(min_real_fee)(min_rf_percent)(extensions) )
FC_REFLECT( graphene::chain::committee_member_create_operation, (fee)(account)(pledge)(url)(extensions) )

FC_REFLECT( graphene::chain::committee_member_update_operation::fee_parameters_type,
            (fee)(min_real_fee)(min_rf_percent)(extensions) )
FC_REFLECT( graphene::chain::committee_member_update_operation, (fee)(account)(new_pledge)(new_url)(extensions) )

FC_REFLECT( graphene::chain::committee_member_vote_update_operation::fee_parameters_type,
            (fee)(min_real_fee)(min_rf_percent)(extensions) )
FC_REFLECT( graphene::chain::committee_member_vote_update_operation,
            (fee)(voter)(committee_members_to_add)(committee_members_to_remove)(extensions) )

FC_REFLECT( graphene::chain::committee_update_account_priviledge_item_type::account_priviledge_update_options,
            (can_vote)(is_admin)(is_registrar)(takeover_registrar) )
FC_REFLECT( graphene::chain::committee_update_account_priviledge_item_type, (account)(new_priviledges) )
FC_REFLECT( graphene::chain::committee_updatable_parameters,
            (maximum_transaction_size)
            (maximum_block_size)
            (maximum_time_until_expiration)
            (maximum_authority_membership)
            (max_authority_depth)
            (csaf_rate)
            (max_csaf_per_account)
            (csaf_accumulate_window)
            (min_witness_pledge)
            (max_witness_pledge_seconds)
            (witness_avg_pledge_update_interval)
            (witness_pledge_release_delay)
            (min_governance_voting_balance)
            //(max_governance_voting_proxy_level)
            (governance_voting_expiration_blocks)
            (governance_votes_update_interval)
            (max_governance_votes_seconds)
            (max_witnesses_voted_per_account)
            (max_witness_inactive_blocks)
            (by_vote_top_witness_pay_per_block)
            (by_vote_rest_witness_pay_per_block)
            (by_pledge_witness_pay_per_block)
            (by_vote_top_witness_count)
            (by_vote_rest_witness_count)
            (by_pledge_witness_count)
            (budget_adjust_interval)
            (budget_adjust_target)
            //(committee_size)
            //(committee_update_interval)
            (min_committee_member_pledge)
            (committee_member_pledge_release_delay)
            //(max_committee_members_voted_per_account)
            (witness_report_prosecution_period)
            (witness_report_allow_pre_last_block)
            (witness_report_pledge_deduction_amount)
            (platform_min_pledge)
            (platform_pledge_release_delay)
            (platform_max_vote_per_account)
          )

FC_REFLECT_TYPENAME( graphene::chain::committee_update_fee_schedule_item_type )
FC_REFLECT_TYPENAME( graphene::chain::committee_update_global_parameter_item_type )
FC_REFLECT_TYPENAME( graphene::chain::committee_proposal_item_type )

FC_REFLECT_ENUM( graphene::chain::voting_opinion_type,
   (opinion_against)
   (opinion_neutral)
   (opinion_for)
   )

FC_REFLECT( graphene::chain::committee_proposal_create_operation::fee_parameters_type,
            (basic_fee)(price_per_item)(min_real_fee)(min_rf_percent)(extensions) )
FC_REFLECT( graphene::chain::committee_proposal_create_operation,
            (fee)(proposer)(items)
            (voting_closing_block_num)(execution_block_num)(expiration_block_num)
            (proposer_opinion)
            (extensions) )

FC_REFLECT( graphene::chain::committee_proposal_update_operation::fee_parameters_type,
            (fee)(min_real_fee)(min_rf_percent)(extensions) )
FC_REFLECT( graphene::chain::committee_proposal_update_operation, (fee)(account)(proposal_number)(opinion)(extensions) )

FC_REFLECT( graphene::chain::committee_member_update_global_parameters_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::committee_member_update_global_parameters_operation, (fee)(new_parameters) );
