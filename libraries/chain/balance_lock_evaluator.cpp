/*
 * Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
 */
#pragma once
#include <graphene/chain/balance_lock_evaluator.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/protocol/chain_parameters.hpp>

namespace graphene { namespace chain {

void_result balance_lock_update_evaluator::do_evaluate(const operation_type& op)
{
   try {
      const database& d = db();
      const dynamic_global_property_object& dpo = d.get_dynamic_global_properties();
      FC_ASSERT(d.head_block_time() >= HARDFORK_0_5_TIME && dpo.enabled_hardfork_version == ENABLE_HEAD_FORK_05, "Can only update balance lock after HARDFORK_0_5_TIME");

      account_stats = &d.get_account_statistics_by_uid(op.account);

      if (op.new_lock_balance> 0)//change lock balance
      {
         FC_ASSERT(op.new_lock_balance != account_stats->locked_balance_for_feepoint, "new_lock_balance specified but did not change");

         auto available_balance = account_stats->core_balance
            - account_stats->core_leased_out
            - account_stats->total_platform_pledge
            - account_stats->total_committee_member_pledge
            - account_stats->total_witness_pledge; // releasing balance can be reused.        
         FC_ASSERT(available_balance >= op.new_lock_balance,
            "Insufficient Balance: account ${a}'s available balance of ${b} is less than required ${r}",
            ("a", op.account)
            ("b", d.to_pretty_core_string(available_balance))
            ("r", d.to_pretty_core_string(op.new_lock_balance)));
      }

      return void_result();

   }FC_CAPTURE_AND_RETHROW((op))
}

void_result balance_lock_update_evaluator::do_apply(const operation_type& op)
{
   try {
      database& d = db();   

      const auto& global_params = d.get_global_properties().parameters;
      const uint64_t csaf_window = global_params.csaf_accumulate_window;
      auto block_time = d.head_block_time();
     
      if (op.new_lock_balance > 0)
      {
         // update account stats
         share_type delta = op.new_lock_balance - account_stats->locked_balance_for_feepoint;
         if (delta > 0)//more locked balance
         {
            d.modify(*account_stats, [&](account_statistics_object& s) {
               if (s.releasing_locked_feepoint > delta)
                  s.releasing_locked_feepoint -= delta;
               else
               {     
                  s.update_coin_seconds_earned(csaf_window, block_time, ENABLE_HEAD_FORK_05);
                  s.locked_balance_for_feepoint = op.new_lock_balance;
                  if (s.releasing_locked_feepoint > 0)
                  {
                     s.releasing_locked_feepoint = 0;
                     s.feepoint_unlock_block_number = -1;
                  }
               }
            });
         }
         else //less locked balance
         {                 
            d.modify(*account_stats, [&](account_statistics_object& s) {
               s.update_coin_seconds_earned(csaf_window, block_time, ENABLE_HEAD_FORK_05);
               s.releasing_locked_feepoint -= delta;
               s.locked_balance_for_feepoint += delta;
               s.feepoint_unlock_block_number = d.head_block_num() + global_params.get_award_params().unlocked_balance_release_delay;
            });
         }
      }

      return void_result();
   } FC_CAPTURE_AND_RETHROW((op))
}

} } // graphene::chain
