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
#include <graphene/chain/protocol/block_header.hpp>

namespace graphene { namespace chain { 

  /**
    * @brief Create a witness object, as a bid to hold a witness position on the network.
    * @ingroup operations
    *
    * Accounts which wish to become witnesses may use this operation to create a witness object which stakeholders may
    * vote on to approve its position as a witness.
    */

   namespace pledge_mining {
      struct ext
      {
         optional<bool>        can_pledge;
         optional<uint32_t>    bonus_rate;
      };
   }

   struct witness_create_operation : public base_operation
   {
      struct fee_parameters_type
      {
         uint64_t fee              = 1000 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee     = 1000 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint16_t min_rf_percent   = 10000;
         extensions_type   extensions;
      };

      fee_type          fee;
      /// The account which owns the witness. This account pays the fee for this operation.
      account_uid_type  account;
      public_key_type   block_signing_key;
      asset             pledge;
      string            url;
      optional< extension<pledge_mining::ext> > extensions;

      account_uid_type  fee_payer_uid()const { return account; }
      void              validate()const;
      //share_type      calculate_fee(const fee_parameters_type& k)const;
      void get_required_active_uid_authorities( flat_set<account_uid_type>& a,bool enabled_hardfork )const
      {
         // need active authority
         a.insert( account );
      }
   };

  /**
    * @brief Update a witness object's URL and block signing key.
    * @ingroup operations
    */
   struct witness_update_operation : public base_operation
   {
      struct fee_parameters_type
      {
         uint64_t fee              = 10 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee     = 0;
         uint16_t min_rf_percent   = 0;
         extensions_type   extensions;
      };

      fee_type          fee;
      /// The account which owns the witness. This account pays the fee for this operation.
      account_uid_type  account;

      /// The new block signing key.
      optional< public_key_type >  new_signing_key;
      /// The new pledge
      optional< asset           >  new_pledge;
      /// The new URL.
      optional< string          >  new_url;

      optional< extension<pledge_mining::ext> > extensions;

      account_uid_type  fee_payer_uid()const { return account; }
      void              validate()const;
      //share_type      calculate_fee(const fee_parameters_type& k)const;
      void get_required_active_uid_authorities( flat_set<account_uid_type>& a,bool enabled_hardfork )const
      {
         // need active authority
         a.insert( account );
      }
   };

   /**
    * @brief Change or refresh witness voting status.
    * @ingroup operations
    */
   struct witness_vote_update_operation : public base_operation
   {
      struct fee_parameters_type
      {
         uint64_t basic_fee         = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t price_per_witness = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee      = 0;
         uint16_t min_rf_percent    = 0;
         extensions_type   extensions;
      };

      fee_type                   fee;
      /// The account which vote for witnesses. This account pays the fee for this operation.
      account_uid_type           voter;
      flat_set<account_uid_type> witnesses_to_add;
      flat_set<account_uid_type> witnesses_to_remove;

      extensions_type   extensions;

      account_uid_type  fee_payer_uid()const { return voter; }
      void              validate()const;
      share_type        calculate_fee(const fee_parameters_type& k)const;
      void get_required_active_uid_authorities( flat_set<account_uid_type>& a ,bool enabled_hardfork)const
      {
         // need active authority
         a.insert( voter );
      }
   };

  /**
    * @brief collects witness pay.
    * @ingroup operations
    */
   struct witness_collect_pay_operation : public base_operation
   {
      struct fee_parameters_type
      {
         uint64_t fee              = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee     = 0;
         uint16_t min_rf_percent   = 0;
         extensions_type   extensions;
      };

      fee_type          fee;
      /// The account which owns the witness. This account pays the fee for this operation.
      account_uid_type  account;

      /// The amount to collect
      asset             pay;

      extensions_type   extensions;

      account_uid_type  fee_payer_uid()const { return account; }
      void              validate()const;
      //share_type      calculate_fee(const fee_parameters_type& k)const;
      void get_required_active_uid_authorities( flat_set<account_uid_type>& a ,bool enabled_hardfork)const
      {
         // need active authority
         a.insert( account );
      }
   };

  /**
    * @brief report a witness that produced two different blocks with same block number
    * @ingroup operations
    */
   struct witness_report_operation : public base_operation
   {
      struct fee_parameters_type
      {
         uint64_t fee              = 0;
         uint64_t min_real_fee     = 0;
         uint16_t min_rf_percent   = 0;
         extensions_type   extensions;
      };

      fee_type            fee;
      account_uid_type    reporter;
      signed_block_header first_block;
      signed_block_header second_block;

      extensions_type   extensions;

      account_uid_type  fee_payer_uid()const { return reporter; }
      void              validate()const;
      //share_type      calculate_fee(const fee_parameters_type& k)const;
      void get_required_secondary_uid_authorities( flat_set<account_uid_type>& a,bool enabled_hardfork )const
      {
         // need secondary authority
         a.insert( reporter );
      }
   };

} } // graphene::chain

FC_REFLECT( graphene::chain::witness_create_operation::fee_parameters_type, (fee)(min_real_fee)(min_rf_percent)(extensions) )
FC_REFLECT( graphene::chain::witness_create_operation, (fee)(account)(block_signing_key)(pledge)(url)(extensions) )

FC_REFLECT( graphene::chain::witness_update_operation::fee_parameters_type, (fee)(min_real_fee)(min_rf_percent)(extensions) )
FC_REFLECT( graphene::chain::witness_update_operation, (fee)(account)(new_signing_key)(new_pledge)(new_url)(extensions) )

FC_REFLECT( graphene::chain::witness_vote_update_operation::fee_parameters_type,
            (basic_fee)(price_per_witness)(min_real_fee)(min_rf_percent)(extensions) )
FC_REFLECT( graphene::chain::witness_vote_update_operation, (fee)(voter)(witnesses_to_add)(witnesses_to_remove)(extensions) )

FC_REFLECT( graphene::chain::witness_collect_pay_operation::fee_parameters_type, (fee)(min_real_fee)(min_rf_percent)(extensions) )
FC_REFLECT( graphene::chain::witness_collect_pay_operation, (fee)(account)(pay)(extensions) )

FC_REFLECT( graphene::chain::witness_report_operation::fee_parameters_type, (fee)(min_real_fee)(min_rf_percent)(extensions) )
FC_REFLECT( graphene::chain::witness_report_operation, (fee)(reporter)(first_block)(second_block)(extensions) )

FC_REFLECT( graphene::chain::pledge_mining::ext, (can_pledge)(bonus_rate))
