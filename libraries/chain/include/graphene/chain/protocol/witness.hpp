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

namespace graphene { namespace chain { 

  /**
    * @brief Create a witness object, as a bid to hold a witness position on the network.
    * @ingroup operations
    *
    * Accounts which wish to become witnesses may use this operation to create a witness object which stakeholders may
    * vote on to approve its position as a witness.
    */
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
      account_uid_type  witness_account;
      public_key_type   block_signing_key;
      asset             pledge;
      string            url;
      extensions_type   extensions;

      account_uid_type  fee_payer_uid()const { return witness_account; }
      void              validate()const;
      //share_type      calculate_fee(const fee_parameters_type& k)const;
      void get_required_active_uid_authorities( flat_set<account_uid_type>& a )const
      {
         // need active authority
         a.insert( witness_account );
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
      account_uid_type  witness_account;

      /// The new block signing key.
      optional< public_key_type >  new_signing_key;
      /// The new pledge
      optional< asset           >  new_pledge;
      /// The new URL.
      optional< string          >  new_url;

      extensions_type   extensions;

      account_uid_type  fee_payer_uid()const { return witness_account; }
      void              validate()const;
      //share_type      calculate_fee(const fee_parameters_type& k)const;
      void get_required_active_uid_authorities( flat_set<account_uid_type>& a )const
      {
         // need active authority
         a.insert( witness_account );
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
      void get_required_active_uid_authorities( flat_set<account_uid_type>& a )const
      {
         // need active authority
         a.insert( voter );
      }
   };

  /**
    * @brief Change or refresh witness voting proxy.
    * @ingroup operations
    */
   struct witness_vote_proxy_operation : public base_operation
   {
      struct fee_parameters_type
      {
         uint64_t fee              = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee     = 0;
         uint16_t min_rf_percent   = 0;
         extensions_type   extensions;
      };

      fee_type                   fee;
      /// The account which vote for witnesses. This account pays the fee for this operation.
      account_uid_type           voter;
      account_uid_type           proxy;

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

} } // graphene::chain

FC_REFLECT( graphene::chain::witness_create_operation::fee_parameters_type, (fee)(min_real_fee)(min_rf_percent)(extensions) )
FC_REFLECT( graphene::chain::witness_create_operation, (fee)(witness_account)(block_signing_key)(pledge)(url)(extensions) )

FC_REFLECT( graphene::chain::witness_update_operation::fee_parameters_type, (fee)(min_real_fee)(min_rf_percent)(extensions) )
FC_REFLECT( graphene::chain::witness_update_operation, (fee)(witness_account)(new_signing_key)(new_pledge)(new_url)(extensions) )

FC_REFLECT( graphene::chain::witness_vote_update_operation::fee_parameters_type,
            (basic_fee)(price_per_witness)(min_real_fee)(min_rf_percent)(extensions) )
FC_REFLECT( graphene::chain::witness_vote_update_operation, (fee)(voter)(witnesses_to_add)(witnesses_to_remove)(extensions) )

FC_REFLECT( graphene::chain::witness_vote_proxy_operation::fee_parameters_type, (fee)(min_real_fee)(min_rf_percent)(extensions) )
FC_REFLECT( graphene::chain::witness_vote_proxy_operation, (fee)(voter)(proxy)(extensions) )
