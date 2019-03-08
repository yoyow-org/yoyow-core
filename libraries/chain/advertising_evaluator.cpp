/*
 * Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
 */
#include <graphene/chain/content_evaluator.hpp>
#include <graphene/chain/content_object.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/is_authorized_asset.hpp>
#include <graphene/chain/protocol/chain_parameters.hpp>
#include <boost/multiprecision/cpp_int.hpp>

namespace graphene { namespace chain {

void_result advertising_create_evaluator::do_evaluate(const operation_type& op)
{
    try {
        const database& d = db();
        FC_ASSERT(d.head_block_time() >= HARDFORK_0_4_TIME, "Can only create advertising after HARDFORK_0_4_TIME");
        d.get_platform_by_owner(op.platform); // make sure pid exists
        return void_result();
    }FC_CAPTURE_AND_RETHROW((op))
}

object_id_type advertising_create_evaluator::do_apply(const operation_type& op)
{
    try {
        database& d = db();
        const auto& advertising_obj = d.create<advertising_object>([&](advertising_object& obj)
        {
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
        advertising_obj = d.find_advertising(op.advertising_id);
        FC_ASSERT(advertising_obj != nullptr, "advertising_object doesn`t exsit");
        FC_ASSERT(advertising_obj->platform == op.platform, "Can`t update other`s advetising. ");

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
      advertising_obj = d.find_advertising(op.advertising_id);
      FC_ASSERT(advertising_obj != nullptr && advertising_obj->platform == op.platform, 
         "advertising ${tid} on platform ${platform} is invalid.",("tid", op.advertising_id)("platform", op.platform));
      FC_ASSERT(advertising_obj->on_sell, "advertising {id} on platform {platform} not on sell", ("id", op.advertising_id)("platform", op.platform));
      FC_ASSERT(op.start_time >= d.head_block_time(), "start time should be later");

      time_point_sec end_time = op.start_time + advertising_obj->unit_time * op.buy_number;
      for (const auto &o : advertising_obj->effective_orders) {
         if (op.start_time >= o.second.end_time || end_time <= o.second.start_time)
            continue;
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

      d.modify(*advertising_obj, [&](advertising_object& obj)
      {
         advertising_object::Advertising_Order order;
         order.user = op.from_account;
         order.start_time = op.start_time;
         order.end_time = op.start_time + obj.unit_time * op.buy_number;
         order.buy_request_time = d.head_block_time();
         order.released_balance = necessary_balance;
         order.extra_data = op.extra_data;
         if (op.memo.valid())
            order.memo = op.memo;

         obj.order_sequence++;
         obj.undetermined_orders.emplace(obj.order_sequence, order);
         obj.last_update_time = d.head_block_time();
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
      advertising_obj = d.find_advertising(op.advertising_id);
      FC_ASSERT(advertising_obj != nullptr && advertising_obj->platform == op.platform,
         "advertising ${tid} on platform ${platform} is invalid.", ("tid", op.advertising_id)("platform", op.platform));
      
      bool found = advertising_obj->undetermined_orders.find(op.order_sequence) != advertising_obj->undetermined_orders.end();
      FC_ASSERT(found, "order {order} is not in undetermined queues", ("order", op.order_sequence));

      if (op.iscomfirm) {
         share_type necessary_balance = advertising_obj->undetermined_orders.at(op.order_sequence).released_balance;
         const auto& params = d.get_global_properties().parameters.get_award_params();
         FC_ASSERT(necessary_balance > params.advertising_confirmed_min_fee,
            "buy price is not enough to pay The lowest poundage ${fee}", ("fee", params.advertising_confirmed_min_fee));
      }

      return void_result();

   }FC_CAPTURE_AND_RETHROW((op))
}

advertising_confirm_result advertising_confirm_evaluator::do_apply(const operation_type& op)
{
   try {
      database& d = db();

      advertising_object::Advertising_Order confirm_order = advertising_obj->undetermined_orders.at(op.order_sequence);
      advertising_confirm_result result;
      if (op.iscomfirm)
      {
         d.modify(*advertising_obj, [&](advertising_object& obj)
         {      
            obj.effective_orders.emplace(confirm_order.start_time, confirm_order);
            obj.effective_orders.at(confirm_order.start_time).released_balance = 0;
            obj.undetermined_orders.erase(op.order_sequence);

            obj.last_update_time = d.head_block_time();
         });

         const auto& params = d.get_global_properties().parameters.get_award_params();
         share_type fee = ((uint128_t)confirm_order.released_balance.value * params.advertising_confirmed_fee_rate
            / GRAPHENE_100_PERCENT).convert_to<int64_t>();
         if (fee < params.advertising_confirmed_min_fee)
            fee = params.advertising_confirmed_min_fee;

         d.adjust_balance(op.platform, asset(confirm_order.released_balance - fee));
         const auto& core_asset = d.get_core_asset();
         const auto& core_dyn_data = core_asset.dynamic_data(d);
         d.modify(core_dyn_data, [&](asset_dynamic_data_object& dyn)
         {
            dyn.current_supply -= fee;
         });

         result.emplace(confirm_order.user, 0);

         auto undermined_order = advertising_obj->undetermined_orders;
         auto itr = undermined_order.begin();
         while (itr != undermined_order.end())
         {
            if (itr->second.start_time >= confirm_order.end_time || itr->second.end_time <= confirm_order.start_time) {
               itr++;
            }
            else
            {
               d.adjust_balance(itr->second.user, asset(itr->second.released_balance));
               result.emplace(itr->second.user, itr->second.released_balance);
               undermined_order.erase(itr++);
            }           
         }

         if (undermined_order.size() != advertising_obj->undetermined_orders.size())
         {
            d.modify(*advertising_obj, [&](advertising_object& obj)
            {
               obj.undetermined_orders = undermined_order;
            });
         }
      }
      else
      {
         d.adjust_balance(confirm_order.user, asset(confirm_order.released_balance));
         d.modify(*advertising_obj, [&](advertising_object& obj)
         {
            obj.undetermined_orders.erase(op.order_sequence);
            obj.last_update_time = d.head_block_time();
         });
         result.emplace(confirm_order.user, confirm_order.released_balance);
      }

      return result;

   } FC_CAPTURE_AND_RETHROW((op))
}

void_result advertising_ransom_evaluator::do_evaluate(const operation_type& op)
{
    try {
        const database& d = db();
        FC_ASSERT(d.head_block_time() >= HARDFORK_0_4_TIME, "Can only ransom advertising after HARDFORK_0_4_TIME");
        d.get_platform_by_owner(op.platform); // make sure pid exists
        d.get_account_by_uid(op.from_account);
        advertising_obj = d.find_advertising(op.advertising_id);
        FC_ASSERT(advertising_obj != nullptr, "advertising_object doesn`t exsit");
        bool isfound = false;
        auto iter_ad = advertising_obj->undetermined_orders.find(op.order_sequence);
        if (iter_ad != advertising_obj->undetermined_orders.end()){
            isfound = true;
            ad_order = &(iter_ad->second);
            FC_ASSERT(ad_order->user == op.from_account, "your can only ransom your own order. ");
            FC_ASSERT(ad_order->buy_request_time + GRAPHENE_ADVERTISING_COMFIRM_TIME < d.head_block_time(), "the buy advertising is undetermined. Can`t ransom now.");
        }
        FC_ASSERT(isfound, "Advertising order isn`t found in advertising_object`s undetermined_orders. ");
        return void_result();

    }FC_CAPTURE_AND_RETHROW((op))
}

void_result advertising_ransom_evaluator::do_apply(const operation_type& op)
{
    try {
        database& d = db();
        share_type sell_price = ad_order->released_balance;
        d.modify(*advertising_obj, [&](advertising_object& obj) {
            auto iter_ad = obj.undetermined_orders.find(op.order_sequence);
            obj.undetermined_orders.erase(iter_ad);
            obj.last_update_time = d.head_block_time();
        });
        d.adjust_balance(op.from_account, asset(sell_price));
    } FC_CAPTURE_AND_RETHROW((op))
}

} } // graphene::chain
