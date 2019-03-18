/*
 * Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
 */
#pragma once
#include <graphene/chain/custom_vote_evaluator.hpp>
#include <graphene/chain/custom_vote_object.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/is_authorized_asset.hpp>
#include <graphene/chain/protocol/chain_parameters.hpp>
#include <graphene/chain/is_authorized_asset.hpp>

namespace graphene { namespace chain {

void_result custom_vote_create_evaluator::do_evaluate(const operation_type& op)
{
   try {
      const database& d = db();
      FC_ASSERT(d.head_block_time() >= HARDFORK_0_4_TIME, "Can only create custom vote after HARDFORK_0_4_TIME");

      d.get_account_by_uid(op.custom_vote_creater);//check create account exist
      account_stats = &d.get_account_statistics_by_uid(op.custom_vote_creater);
      FC_ASSERT((account_stats->last_custom_vote_sequence + 1) == op.vote_vid,"vote_vid ${vid} is invalid.",("vid", op.vote_vid)); 

      const auto& asset_indx = d.get_index_type<asset_index>().indices().get<by_aid>();
      auto asset_aid_itr = asset_indx.find(op.vote_asset_id);
      FC_ASSERT(asset_aid_itr != asset_indx.end(), "Asset '${aid}' is not existent", ("aid", op.vote_asset_id));

      const auto& params = d.get_global_properties().parameters.get_award_params();
      auto range_end_time = d.head_block_time() + params.custom_vote_effective_time;
      FC_ASSERT(op.vote_expired_time > d.head_block_time() && op.vote_expired_time < range_end_time,
         "vote expired time should in range ${start}--${end}", ("start", d.head_block_time())("end", range_end_time));

      return void_result();

   }FC_CAPTURE_AND_RETHROW((op))
}

object_id_type custom_vote_create_evaluator::do_apply(const operation_type& op)
{
   try {
      database& d = db();
      const auto& custom_vote_obj = d.create<custom_vote_object>([&](custom_vote_object& obj)
      {
         obj.custom_vote_creater = op.custom_vote_creater;
         obj.vote_vid = op.vote_vid;
         obj.title = op.title;
         obj.description = op.description;
         obj.vote_expired_time = op.vote_expired_time;
         obj.vote_asset_id = op.vote_asset_id;
         obj.required_asset_amount = op.required_asset_amount;
         obj.minimum_selected_items = op.minimum_selected_items;
         obj.maximum_selected_items = op.maximum_selected_items;

         obj.vote_result.resize(op.options.size());
         obj.options = op.options;
      });

      d.modify(*account_stats, [&](account_statistics_object& s) {
         s.last_custom_vote_sequence += 1;
      });

      return custom_vote_obj.id;
   } FC_CAPTURE_AND_RETHROW((op))
}

void_result custom_vote_cast_evaluator::do_evaluate(const operation_type& op)
{
   try {
      const database& d = db();
      FC_ASSERT(d.head_block_time() >= HARDFORK_0_4_TIME, "Can only cast custom vote after HARDFORK_0_4_TIME");

      d.get_account_by_uid(op.voter);//check custom voter account exist
       
      custom_vote_obj = d.find_custom_vote_by_vid(op.custom_vote_creater, op.custom_vote_vid);
      FC_ASSERT(custom_vote_obj != nullptr, "custom vote ${vid} not found.", ("id", op.custom_vote_vid));
      FC_ASSERT(d.head_block_time() <= custom_vote_obj->vote_expired_time, "custom vote already overdue");
      FC_ASSERT(op.vote_result.size() >= custom_vote_obj->minimum_selected_items && op.vote_result.size() <= custom_vote_obj->maximum_selected_items, 
         "vote options num is not in range ${min} - ${max}.", ("min", custom_vote_obj->minimum_selected_items)("max", custom_vote_obj->maximum_selected_items));

      votes = d.get_balance(op.voter, custom_vote_obj->vote_asset_id).amount;
      FC_ASSERT(votes >= custom_vote_obj->required_asset_amount, "asset ${aid} balance less than required amount for vote ${amount}", 
         ("aid", custom_vote_obj->vote_asset_id)("amount", custom_vote_obj->required_asset_amount));
      
      const auto& idx = d.get_index_type<cast_custom_vote_index>().indices().get<by_custom_voter>();
      auto itr = idx.find(std::make_tuple(op.voter, op.custom_vote_creater, op.custom_vote_vid));
      FC_ASSERT(itr == idx.end(), "account ${uid} already cast a vote for custom vote ${vid}", ("uid", op.voter)("vid", op.custom_vote_vid));

      for (const auto& index : op.vote_result)
      {
         FC_ASSERT(index < custom_vote_obj->options.size(), "option ${item} is not existent", ("item", index));
      }
         
      return void_result();
   }FC_CAPTURE_AND_RETHROW((op))
}

object_id_type custom_vote_cast_evaluator::do_apply(const operation_type& op)
{
   try {
      database& d = db();
      const auto& cast_vote_obj = d.create<cast_custom_vote_object>([&](cast_custom_vote_object& obj)
      {
         obj.voter = op.voter;
         obj.custom_vote_creater = op.custom_vote_creater;
         obj.custom_vote_vid = op.custom_vote_vid;
         obj.vote_result = op.vote_result;
      });

      d.modify(*custom_vote_obj, [&](custom_vote_object& obj) 
      {
         for (const auto& v : op.vote_result)
            obj.vote_result.at(v) += votes.value;
      });

      return cast_vote_obj.id;
   } FC_CAPTURE_AND_RETHROW((op))
}

} } // graphene::chain
