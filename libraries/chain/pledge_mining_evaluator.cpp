/*
* Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
*/
#include <graphene/chain/pledge_mining_evaluator.hpp>
#include <graphene/chain/pledge_mining_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/hardfork.hpp>

namespace graphene { namespace chain {

void_result pledge_mining_update_evaluator::do_evaluate(const pledge_mining_update_operation& op)
{
   try {
      database& d = db();
      FC_ASSERT(d.head_block_time() >= HARDFORK_0_5_TIME, "Can only account update pledge to witness after HARDFORK_0_5_TIME");
      account_stats = &d.get_account_statistics_by_uid(op.pledge_account); // check if the pledge account exists
      witness_obj = &d.get_witness_by_uid(op.witness);
      FC_ASSERT(witness_obj->can_pledge, "witness ${witness} can not pledge", ("witness", op.witness));
      pledge_mining_obj = d.find_pledge_mining(op.witness, op.pledge_account);
      
      if (pledge_mining_obj)
      {
         FC_ASSERT(pledge_mining_obj->pledge > 0, "account ${account} already cancel pledge to witness {witness}, pledge is not currently allowed",
            ("account", op.pledge_account)
            ("witness", op.witness));
         FC_ASSERT(pledge_mining_obj->witness == op.witness, "account ${account} already pledge to witness {witness}, can not pledge to other",
            ("account", op.pledge_account)
            ("witness", pledge_mining_obj->witness));
      }

      if (op.new_pledge > 0)
      {
         auto min_pledge_to_witness = d.get_global_properties().parameters.get_award_params().min_mining_pledge;
         FC_ASSERT(op.new_pledge.value >= min_pledge_to_witness,
                   "Insufficient pledge: provided ${p}, need ${r}",
                   ("p", d.to_pretty_core_string(op.new_pledge))
                   ("r", d.to_pretty_core_string(min_pledge_to_witness)));

         if (pledge_mining_obj)
            FC_ASSERT(op.new_pledge.value != pledge_mining_obj->pledge, "new_pledge specified but did not change");

         auto available_balance = account_stats->core_balance
                                - account_stats->core_leased_out
                                - account_stats->total_platform_pledge
                                - account_stats->locked_balance_for_feepoint
                                - account_stats->releasing_locked_feepoint
                                - account_stats->total_committee_member_pledge
                                - account_stats->total_witness_pledge;

         FC_ASSERT(available_balance+pledge_mining_obj->total_unrelease_pledge() >= op.new_pledge,
            "Insufficient Balance: account ${a}'s available balance of ${b} is less than required ${r}",
            ("a", op.pledge_account)
            ("b", d.to_pretty_core_string(available_balance))
            ("r", d.to_pretty_string(op.new_pledge)));
      }
      else if (op.new_pledge == 0)
      {
         FC_ASSERT(pledge_mining_obj != nullptr, "Cancel nonexistent pledge is not allowed");
      }
      
      return void_result();
   } FC_CAPTURE_AND_RETHROW((op))
}

void_result pledge_mining_update_evaluator::do_apply(const pledge_mining_update_operation& op)
{
   try {
      database& d = db();

      const auto& params = d.get_global_properties().parameters.get_award_params();
      const dynamic_global_property_object& dpo = d.get_dynamic_global_properties();

      share_type delta_pledge_to_witness = 0;
      share_type send_bonus = 0;
      if (pledge_mining_obj)
      {
         send_bonus = d.update_pledge_mining_bonus_to_account(*witness_obj, *pledge_mining_obj);
         share_type delta_available_balance=0;
         delta_pledge_to_witness=op.new_pledge-pledge_mining_obj->pledge;
         d.modify(*pledge_mining_obj, [&](pledge_mining_object& obj) {
            delta_available_balance=obj.update_pledge(op.new_pledge, d.head_block_num() + params.mining_pledge_release_delay);
         });
         d.modify(*account_stats, [&](account_statistics_object& s) {
            s.total_mining_pledge += delta_available_balance;
         });
      }
      else//create pledge witness object
      {
         d.create<pledge_mining_object>([&](pledge_mining_object& obj){
            obj.pledge_account = op.pledge_account;
            obj.witness = op.witness;
            obj.pledge = op.new_pledge.value;
            obj.total_mining_pledge = op.new_pledge.value;
            if (!witness_obj->bonus_per_pledge.empty())
               obj.last_bonus_block_num = witness_obj->bonus_per_pledge.rbegin()->first;
         });

         d.modify(*account_stats, [&](account_statistics_object& s) {
            s.total_mining_pledge = op.new_pledge;
         });

         delta_pledge_to_witness = op.new_pledge;
      }

      //update witness object
      d.modify(*witness_obj, [&](witness_object& obj) {
         obj.total_mining_pledge += delta_pledge_to_witness.value;
         obj.is_pledge_changed = true;
         if (send_bonus > 0)
            obj.already_distribute_bonus += send_bonus;
      });
      //update dynamic global property object
      d.modify(dpo, [&](dynamic_global_property_object& _dpo) {
         _dpo.total_witness_pledge += delta_pledge_to_witness;
      });

      return void_result();
   } FC_CAPTURE_AND_RETHROW((op))
}

void_result pledge_bonus_collect_evaluator::do_evaluate(const pledge_bonus_collect_operation& op)
{
   try {
      database& d = db();
      account_stats = &d.get_account_statistics_by_uid(op.account);

      FC_ASSERT(account_stats->uncollected_pledge_bonus >= op.bonus,
         "Can not collect so much: have ${b}, requested ${r}",
         ("b", d.to_pretty_core_string(account_stats->uncollected_pledge_bonus))
         ("r", d.to_pretty_core_string(op.bonus)));

      return void_result();
   } FC_CAPTURE_AND_RETHROW((op))
}

void_result pledge_bonus_collect_evaluator::do_apply(const pledge_bonus_collect_operation& op)
{
   try {
      database& d = db();

      d.adjust_balance(op.account, asset(op.bonus));
      d.modify(*account_stats, [&](account_statistics_object& s) {
         s.uncollected_witness_pay -= op.bonus;
      });

      return void_result();
   } FC_CAPTURE_AND_RETHROW((op))
}

} } // graphene::chain
