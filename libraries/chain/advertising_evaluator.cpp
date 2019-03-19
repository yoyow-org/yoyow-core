/*
 * Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
 */
#include <graphene/chain/advertising_evaluator.hpp>
#include <graphene/chain/advertising_object.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/is_authorized_asset.hpp>
#include <graphene/chain/protocol/chain_parameters.hpp>
#include <boost/multiprecision/cpp_int.hpp>

typedef boost::multiprecision::uint128_t uint128_t;

namespace graphene { namespace chain {

void_result advertising_create_evaluator::do_evaluate(const operation_type& op)
{
    try {
        const database& d = db();
        FC_ASSERT(d.head_block_time() >= HARDFORK_0_4_TIME, "Can only create advertising after HARDFORK_0_4_TIME");
        platform_obj = d.find_platform_by_owner(op.platform); // make sure platform exists
        FC_ASSERT(platform_obj != nullptr, "platform doesn`t exsit. ");
        FC_ASSERT((platform_obj->last_advertising_sequence + 1) == op.advertising_aid,"advertising_aid ${pid} is invalid.",("pid", op.advertising_aid));
        return void_result();
    }FC_CAPTURE_AND_RETHROW((op))
}

object_id_type advertising_create_evaluator::do_apply(const operation_type& op)
{
    try {
        database& d = db();
        d.modify(*platform_obj, [&](platform_object& s) {
            s.last_advertising_sequence += 1;
        });
        const auto& advertising_obj = d.create<advertising_object>([&](advertising_object& obj)
        {
            obj.advertising_aid = op.advertising_aid;
            obj.platform = op.platform;
            obj.on_sell = true;
            obj.unit_time = op.unit_time;
            obj.unit_price = op.unit_price;
            obj.description = op.description;

            obj.publish_time = d.head_block_time();
            obj.last_update_time = d.head_block_time();
        });
        return advertising_obj.id;
    } FC_CAPTURE_AND_RETHROW((op))
}

void_result advertising_update_evaluator::do_evaluate(const operation_type& op)
{
    try {
        const database& d = db();
        FC_ASSERT(d.head_block_time() >= HARDFORK_0_4_TIME, "Can only update advertising after HARDFORK_0_4_TIME");
        d.get_platform_by_owner(op.platform); // make sure pid exists
        advertising_obj = d.find_advertising(op.platform, op.advertising_aid);
        FC_ASSERT(advertising_obj != nullptr, "advertising_object doesn`t exsit");

        if (op.on_sell.valid())
            FC_ASSERT(*(op.on_sell) != advertising_obj->on_sell, "advertising state needn`t update. ");
        
        return void_result();

    }FC_CAPTURE_AND_RETHROW((op))
}

void_result advertising_update_evaluator::do_apply(const operation_type& op)
{
    try {
        database& d = db();
        d.modify(*advertising_obj, [&](advertising_object& ad) {
            if (op.description.valid())
                ad.description = *(op.description);
            if (op.unit_price.valid())
                ad.unit_price = *(op.unit_price);
            if (op.unit_time.valid())
                ad.unit_time = *(op.unit_time);
            if (op.on_sell.valid())
                ad.on_sell = *(op.on_sell);
            ad.last_update_time = d.head_block_time();
        });

        return void_result();

    } FC_CAPTURE_AND_RETHROW((op))
}

void_result advertising_buy_evaluator::do_evaluate(const operation_type& op)
{
   try {
      const database& d = db();

      FC_ASSERT(d.head_block_time() >= HARDFORK_0_4_TIME, "Can only buy advertising after HARDFORK_0_4_TIME");
      advertising_obj = d.find_advertising(op.platform, op.advertising_aid);
      FC_ASSERT(advertising_obj != nullptr && advertising_obj->platform == op.platform, 
         "advertising ${tid} on platform ${platform} is invalid.",("tid", op.advertising_aid)("platform", op.platform));
      FC_ASSERT(advertising_obj->on_sell, "advertising ${id} on platform ${platform} not on sell", ("id", op.advertising_aid)("platform", op.platform));
      FC_ASSERT(op.start_time >= d.head_block_time(), "start time should be later");
      FC_ASSERT((advertising_obj->last_order_sequence + 1) == op.advertising_order_oid, "advertising_order_oid ${pid} is invalid.", ("pid", op.advertising_order_oid));
      
      const auto& idx = d.get_index_type<advertising_order_index>().indices().get<by_advertising_order_state>();
      auto itr = idx.lower_bound(std::make_tuple(advertising_accepted, op.platform, op.advertising_aid));

      FC_ASSERT(advertising_obj->unit_time <= 10*365*24*60*60 / op.buy_number, "advertising purchasing time should not more than ten years ");

      time_point_sec end_time = op.start_time + advertising_obj->unit_time * op.buy_number;
      while (itr != idx.end() && itr->advertising_aid == op.advertising_aid && itr->platform == op.platform && itr->status == advertising_accepted) {
         if (op.start_time >= itr->end_time || end_time <= itr->start_time) {
            itr++;
            continue;
         }       
         FC_ASSERT(false, "purchasing date have a conflict, buy advertising failed");
      }

      const auto& from_balance = d.get_balance(op.from_account, GRAPHENE_CORE_ASSET_AID);
      necessary_balance = advertising_obj->unit_price * op.buy_number;
      FC_ASSERT(from_balance.amount >= necessary_balance,
         "Insufficient Balance: ${balance}, not enough to buy advertising ${tid} that ${need} needed.",
         ("need", necessary_balance)("balance", d.to_pretty_string(from_balance)));

      const auto& params = d.get_global_properties().parameters.get_award_params();
      FC_ASSERT(necessary_balance > params.advertising_confirmed_min_fee, 
         "buy price is not enough to pay The lowest poundage ${fee}", ("fee", params.advertising_confirmed_min_fee));

      return void_result();

   }FC_CAPTURE_AND_RETHROW((op))
}

asset advertising_buy_evaluator::do_apply(const operation_type& op)
{
   try {
      database& d = db();
      d.modify(*advertising_obj, [&](advertising_object& s) {
          s.last_order_sequence += 1;
      });
      const auto& advertising_order_obj = d.create<advertising_order_object>([&](advertising_order_object& obj)
      {
          obj.advertising_order_oid = op.advertising_order_oid;
         obj.advertising_aid = advertising_obj->advertising_aid;
         obj.platform = op.platform;
         obj.user = op.from_account;
         obj.start_time = op.start_time;
         obj.end_time = op.start_time + advertising_obj->unit_time * op.buy_number;
         obj.buy_request_time = d.head_block_time();
         obj.status = advertising_undetermined;
         obj.released_balance = necessary_balance;
         obj.extra_data = op.extra_data;

         if (op.memo.valid())
            obj.memo = op.memo;
      });

      d.adjust_balance(op.from_account, -asset(necessary_balance));

      return asset(necessary_balance);

   } FC_CAPTURE_AND_RETHROW((op))
}

void_result advertising_confirm_evaluator::do_evaluate(const operation_type& op)
{
   try {
      const database& d = db();

      FC_ASSERT(d.head_block_time() >= HARDFORK_0_4_TIME, "Can only advertising comfirm after HARDFORK_0_4_TIME");
      const auto& advertising_obj = d.find_advertising(op.platform, op.advertising_aid);
      FC_ASSERT(advertising_obj != nullptr && advertising_obj->platform == op.platform,
         "advertising ${tid} on platform ${platform} is invalid.", ("tid", op.advertising_aid)("platform", op.platform));

      advertising_order_obj = d.find_advertising_order(op.platform, op.advertising_aid, op.advertising_order_oid);
      FC_ASSERT(advertising_order_obj != nullptr, "order ${p}_${ad}_${order} is not existent", ("p",op.platform)("ad",op.advertising_aid)("order", op.advertising_order_oid));
      FC_ASSERT(advertising_order_obj->status == advertising_undetermined,
          "order ${p}_${ad}_${order} already effective or refused.", ("p",op.platform)("ad",op.advertising_aid)("order", op.advertising_order_oid));

      if (op.iscomfirm) {
         const auto& params = d.get_global_properties().parameters.get_award_params();
         FC_ASSERT(advertising_order_obj->released_balance > params.advertising_confirmed_min_fee,
            "buy price is not enough to pay the lowest poundage ${fee}", ("fee", params.advertising_confirmed_min_fee));
      }

      return void_result();

   }FC_CAPTURE_AND_RETHROW((op))
}

advertising_confirm_result advertising_confirm_evaluator::do_apply(const operation_type& op)
{
   try {
      database& d = db();

      advertising_confirm_result result;
      if (op.iscomfirm)
      {
         share_type released_balance = advertising_order_obj->released_balance;
         d.modify(*advertising_order_obj, [&](advertising_order_object& obj)
         {
            obj.status = advertising_accepted;
            obj.released_balance = 0;
            obj.handle_time = d.head_block_time();
         });

         const auto& params = d.get_global_properties().parameters.get_award_params();
         share_type fee = ((uint128_t)(released_balance.value)* params.advertising_confirmed_fee_rate
            / GRAPHENE_100_PERCENT).convert_to<int64_t>();
         if (fee < params.advertising_confirmed_min_fee)
            fee = params.advertising_confirmed_min_fee;

         d.adjust_balance(op.platform, asset(released_balance - fee));
         const auto& core_asset = d.get_core_asset();
         const auto& core_dyn_data = core_asset.dynamic_data(d);
         d.modify(core_dyn_data, [&](asset_dynamic_data_object& dyn)
         {
            dyn.current_supply -= fee;
         });

         result.emplace(advertising_order_obj->user, 0);

         const auto& idx = d.get_index_type<advertising_order_index>().indices().get<by_advertising_order_state>();
         auto itr = idx.lower_bound(std::make_tuple(advertising_undetermined, op.platform, op.advertising_aid));

         while (itr != idx.end() && itr->platform == op.platform && itr->advertising_aid == op.advertising_aid && itr->status == advertising_undetermined)
         {
            if (itr->start_time >= advertising_order_obj->end_time || itr->end_time <= advertising_order_obj->start_time) {
               itr++;
            }
            else
            {
               d.adjust_balance(itr->user, asset(itr->released_balance));
               result.emplace(itr->user, itr->released_balance);              
               d.modify(*itr, [&](advertising_order_object& obj)
               {
                  obj.status = advertising_refused;
                  obj.released_balance = 0;
                  obj.handle_time = d.head_block_time();
               });
               itr++;
            }           
         } 
      }
      else
      {
         d.adjust_balance(advertising_order_obj->user, asset(advertising_order_obj->released_balance));
         result.emplace(advertising_order_obj->user, advertising_order_obj->released_balance);

         d.modify(*advertising_order_obj, [&](advertising_order_object& obj)
         {
            obj.status = advertising_refused;
            obj.released_balance = 0;
            obj.handle_time = d.head_block_time();
         });
      }

      return result;

   } FC_CAPTURE_AND_RETHROW((op))
}

void_result advertising_ransom_evaluator::do_evaluate(const operation_type& op)
{
    try {
        const database& d = db();
        FC_ASSERT(d.head_block_time() >= HARDFORK_0_4_TIME, "Can only ransom advertising after HARDFORK_0_4_TIME");
        d.get_platform_by_owner(op.platform); // make sure platorm exists
        d.get_account_by_uid(op.from_account);
        const auto& advertising_obj = d.find_advertising(op.platform, op.advertising_aid);
        FC_ASSERT(advertising_obj != nullptr, "advertising object doesn`t exsit");

        advertising_order_obj = d.find_advertising_order(op.platform, op.advertising_aid, op.advertising_order_oid);
        FC_ASSERT(advertising_order_obj != nullptr, "order ${p}_${ad}_${order} is not existent", ("p", op.platform)("ad", op.advertising_aid)("order", op.advertising_order_oid));
        FC_ASSERT(advertising_order_obj->user == op.from_account, "your can only ransom your own order. ");
        FC_ASSERT(advertising_order_obj->buy_request_time + GRAPHENE_ADVERTISING_COMFIRM_TIME < d.head_block_time(), 
           "the buy advertising is undetermined. Can`t ransom now.");

        return void_result();

    }FC_CAPTURE_AND_RETHROW((op))
}

void_result advertising_ransom_evaluator::do_apply(const operation_type& op)
{
    try {
        database& d = db();

        d.adjust_balance(op.from_account, asset(advertising_order_obj->released_balance));

        d.modify(*advertising_order_obj, [&](advertising_order_object& obj)
        {
           obj.status = advertising_ransom;
           obj.released_balance = 0;
           obj.handle_time = d.head_block_time();
        });

    } FC_CAPTURE_AND_RETHROW((op))
}

} } // graphene::chain
