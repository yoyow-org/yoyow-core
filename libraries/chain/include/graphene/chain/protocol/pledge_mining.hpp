/*
* Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
*/
#pragma once
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/block_header.hpp>

namespace graphene { namespace chain { 

   /**
   * @brief common account pledge asset to witness, 
   * @brief part of witness pay is divided among common accounts according to the pledge amount.
   * @ingroup operations
   */
   struct pledge_mining_update_operation : public base_operation
   {
      struct fee_parameters_type
      {
         uint64_t fee = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee = 0;
         uint16_t min_rf_percent = 0;
         extensions_type   extensions;
      };

      fee_type            fee;
      account_uid_type    pledge_account;
      account_uid_type    witness;
      share_type          new_pledge;

      extensions_type     extensions;

      account_uid_type  fee_payer_uid()const { return pledge_account; }
      void              validate()const;
      void get_required_active_uid_authorities(flat_set<account_uid_type>& a, bool enabled_hardfork)const
      {
         // need active authority
         a.insert(pledge_account);
      }
   };


   /**
   * @brief collects pledge mining bonus.
   * @ingroup operations
   */
   struct pledge_bonus_collect_operation : public base_operation
   {
      struct fee_parameters_type
      {
         uint64_t fee = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee = 0;
         uint16_t min_rf_percent = 0;
         extensions_type   extensions;
      };

      fee_type          fee;
      account_uid_type  account;

      /// The amount to collect
      share_type        bonus;

      extensions_type   extensions;

      account_uid_type  fee_payer_uid()const { return account; }
      void              validate()const;
      void get_required_active_uid_authorities(flat_set<account_uid_type>& a, bool enabled_hardfork)const
      {
         // need active authority
         a.insert(account);
      }
   };

} } // graphene::chain

FC_REFLECT( graphene::chain::pledge_mining_update_operation::fee_parameters_type, (fee)(min_real_fee)(min_rf_percent)(extensions))
FC_REFLECT( graphene::chain::pledge_mining_update_operation, (fee)(pledge_account)(witness)(new_pledge)(extensions))

FC_REFLECT( graphene::chain::pledge_bonus_collect_operation::fee_parameters_type, (fee)(min_real_fee)(min_rf_percent)(extensions))
FC_REFLECT( graphene::chain::pledge_bonus_collect_operation, (fee)(account)(bonus)(extensions))

