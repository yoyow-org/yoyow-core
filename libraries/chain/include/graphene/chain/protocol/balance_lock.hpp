/*
* Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
*/
#pragma once
#include <graphene/chain/protocol/base.hpp>

namespace graphene { namespace chain {

   /**
   * @brief Update a account locked balance
   * locked balance can produce csaf after HARDFORK_0_5_TIME
   * @ingroup operations
   */
   struct balance_lock_update_operation : public base_operation
   {
      struct fee_parameters_type
      {
         uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION / 10;
         uint64_t min_real_fee = 0;
         uint16_t min_rf_percent = 0;
         extensions_type   extensions;
      };

      fee_type          fee;
      /// The account that lock balance. This account pays the fee for this operation.
      account_uid_type  account;

      /// The new lock balance
      share_type        new_lock_balance;

      extensions_type   extensions;

      account_uid_type  fee_payer_uid()const { return account; }
      void              validate()const;
      void get_required_active_uid_authorities(flat_set<account_uid_type>& a, bool enabled_hardfork)const
      {
         // need active authority
         a.insert(account);
      }
   };
}} // graphene::chain



FC_REFLECT( graphene::chain::balance_lock_update_operation::fee_parameters_type, (fee)(min_real_fee)(min_rf_percent)(extensions))
FC_REFLECT( graphene::chain::balance_lock_update_operation, (fee)(account)(new_lock_balance)(extensions))