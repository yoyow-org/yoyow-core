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
      FC_ASSERT(dpo.enabled_hardfork_version >= ENABLE_HEAD_FORK_05, "Can only update balance lock after HARDFORK_0_5_TIME");

      account_stats = &d.get_account_statistics_by_uid(op.account);
      if (account_stats->pledge_balance_ids.count(pledge_balance_type::Lock_balance))
         pledge_balance_obj = &d.get(account_stats->pledge_balance_ids.at(pledge_balance_type::Lock_balance));

      if (op.new_lock_balance> 0)//change lock balance
      {
         if (pledge_balance_obj)
            FC_ASSERT(op.new_lock_balance != pledge_balance_obj->pledge, "new_lock_balance specified but did not change");

         // releasing  locked balance can be reused.
         auto available_balance = account_stats->get_available_core_balance(pledge_balance_type::Lock_balance, d);
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

      d.modify(*account_stats, [&](_account_statistics_object& s) {
         s.update_coin_seconds_earned(csaf_window, block_time,d, ENABLE_HEAD_FORK_05);
      });

      if (pledge_balance_obj != nullptr) 
      {
         const auto& lock_balance_obj = d.get(account_stats->pledge_balance_ids.at(pledge_balance_type::Lock_balance));
         uint32_t new_relase_num = d.head_block_num() + global_params.get_award_params().unlocked_balance_release_delay;
         d.modify(lock_balance_obj, [&](pledge_balance_object& obj) {
            obj.update_pledge(op.new_lock_balance, new_relase_num);
        });
      } 
      else {//create pledge_balance_obj
         const auto& new_pledge_balance_obj = d.create<pledge_balance_object>([&](pledge_balance_object& obj){
            obj.superior_index = op.account;
            obj.type = pledge_balance_type::Lock_balance;
            obj.asset_id = GRAPHENE_CORE_ASSET_AID;
            obj.pledge = op.new_lock_balance;
         });
         d.modify(*account_stats, [&](_account_statistics_object& s) {
            s.pledge_balance_ids.emplace(pledge_balance_type::Lock_balance, new_pledge_balance_obj.id);
         });
      }

      return void_result();
   } FC_CAPTURE_AND_RETHROW((op))
}

} } // graphene::chain
