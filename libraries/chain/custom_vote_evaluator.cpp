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

namespace graphene { namespace chain {

void_result custom_vote_create_evaluator::do_evaluate(const operation_type& op)
{
   try {
      const database& d = db();
      FC_ASSERT(d.head_block_time() >= HARDFORK_0_4_TIME, "Can only create advertising after HARDFORK_0_4_TIME");
      return void_result();
   }FC_CAPTURE_AND_RETHROW((op))
}

object_id_type custom_vote_create_evaluator::do_apply(const operation_type& op)
{
   try {
      database& d = db();
      const auto& custom_vote_obj = d.create<custom_vote_object>([&](custom_vote_object& obj)
      {
      });
      return custom_vote_obj.id;
   } FC_CAPTURE_AND_RETHROW((op))
}

void_result custom_vote_cast_evaluator::do_evaluate(const operation_type& op)
{
   try {
      const database& d = db();
      FC_ASSERT(d.head_block_time() >= HARDFORK_0_4_TIME, "Can only create advertising after HARDFORK_0_4_TIME");
      return void_result();
   }FC_CAPTURE_AND_RETHROW((op))
}

object_id_type custom_vote_cast_evaluator::do_apply(const operation_type& op)
{
   try {
      database& d = db();
      const auto& cast_vote_obj = d.create<cast_custom_vote_object>([&](cast_custom_vote_object& obj)
      {
      });
      return cast_vote_obj.id;
   } FC_CAPTURE_AND_RETHROW((op))
}

} } // graphene::chain
