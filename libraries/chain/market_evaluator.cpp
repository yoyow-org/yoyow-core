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
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/market_object.hpp>

#include <graphene/chain/market_evaluator.hpp>

#include <graphene/chain/database.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/is_authorized_asset.hpp>

#include <graphene/chain/protocol/market.hpp>

#include <fc/uint128.hpp>

namespace graphene { namespace chain {
void_result limit_order_create_evaluator::do_evaluate(const limit_order_create_operation& op)
{ try {
   const database& d = db();
   FC_ASSERT(d.head_block_time() >= HARDFORK_0_5_TIME, "Can only create limit order after HARDFORK_0_5_TIME");
   FC_ASSERT( op.expiration >= d.head_block_time() );

   _seller = &d.get_account_by_uid(op.seller);//_seller        = this->fee_paying_account;
   _sell_asset = &d.get_asset_by_aid(op.amount_to_sell.asset_id);//_sell_asset    = &op.amount_to_sell.asset_id(d);
   _receive_asset = &d.get_asset_by_aid(op.min_to_receive.asset_id);//_receive_asset = &op.min_to_receive.asset_id(d);

   if( _sell_asset->options.whitelist_markets.size() )
      FC_ASSERT( _sell_asset->options.whitelist_markets.find(_receive_asset->asset_id) 
            != _sell_asset->options.whitelist_markets.end(),
            "This market has not been whitelisted." );
   if( _sell_asset->options.blacklist_markets.size() )
      FC_ASSERT( _sell_asset->options.blacklist_markets.find(_receive_asset->asset_id) 
            == _sell_asset->options.blacklist_markets.end(),
            "This market has been blacklisted." );

   //FC_ASSERT( is_authorized_asset( d, *_seller, *_sell_asset ) );
   //FC_ASSERT( is_authorized_asset( d, *_seller, *_receive_asset ) );

   validate_authorized_asset(d, *_seller, *_sell_asset, "'sell' ");
   validate_authorized_asset(d, *_seller, *_receive_asset, "'to' ");

   FC_ASSERT( d.get_balance( *_seller, *_sell_asset ) >= op.amount_to_sell, "insufficient balance",
              ("balance",d.get_balance(*_seller,*_sell_asset))("amount_to_sell",op.amount_to_sell) );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

//void limit_order_create_evaluator::convert_fee()
//{
//   if( db().head_block_time() <= HARDFORK_CORE_604_TIME )
//      generic_evaluator::convert_fee();
//   else
//      if( !trx_state->skip_fee )
//      {
//         if( fee_asset->get_id() != asset_id_type() )
//         {
//            db().modify(*fee_asset_dyn_data, [this](asset_dynamic_data_object& d) {
//               d.fee_pool -= core_fee_paid;
//            });
//         }
//      }
//}
//
//void limit_order_create_evaluator::pay_fee()
//{
//   if( db().head_block_time() <= HARDFORK_445_TIME )
//      generic_evaluator::pay_fee();
//   else
//   {
//      _deferred_fee = core_fee_paid;
//      if( db().head_block_time() > HARDFORK_CORE_604_TIME && fee_asset->get_id() != asset_id_type() )
//         _deferred_paid_fee = fee_from_account;
//   }
//}

object_id_type limit_order_create_evaluator::do_apply(const limit_order_create_operation& op)
{ try {
   const auto& seller_stats = _seller->statistics(db());
   db().modify(seller_stats, [&](account_statistics_object& bal) {
       if( op.amount_to_sell.asset_id == GRAPHENE_CORE_ASSET_AID )
       {
          bal.total_core_in_orders += op.amount_to_sell.amount;
       }
   });

   db().adjust_balance(op.seller, -op.amount_to_sell);

   const auto& new_order_object = db().create<limit_order_object>([&](limit_order_object& obj){
       obj.seller   = _seller->uid;
       obj.for_sale = op.amount_to_sell.amount;
       obj.sell_price = op.get_price();
       obj.expiration = op.expiration;
       //obj.deferred_fee = total_fee_paid; //_deferred_fee;
       //obj.deferred_paid_fee = _deferred_paid_fee;
   });
   limit_order_id_type order_id = new_order_object.id; // save this because we may remove the object by filling it
   bool filled;

   filled = db().apply_order(new_order_object);

   FC_ASSERT( !op.fill_or_kill || filled );

   return order_id;
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result limit_order_cancel_evaluator::do_evaluate(const limit_order_cancel_operation& o)
{ try {
   database& d = db();
   FC_ASSERT(d.head_block_time() >= HARDFORK_0_5_TIME, "Can only cancel limit order after HARDFORK_0_5_TIME");
   _order = &o.order(d);
   FC_ASSERT( _order->seller == o.fee_paying_account );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

asset limit_order_cancel_evaluator::do_apply(const limit_order_cancel_operation& o)
{ try {
   database& d = db();

   auto base_asset = _order->sell_price.base.asset_id;
   auto quote_asset = _order->sell_price.quote.asset_id;
   auto refunded = _order->amount_for_sale();

   d.cancel_limit_order(*_order, false /* don't create a virtual op*/);

   return refunded;
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result market_fee_collect_evaluator::do_evaluate(const market_fee_collect_operation& o)
{
   try {
      database& d = db();
      FC_ASSERT(d.head_block_time() >= HARDFORK_0_5_TIME, "Can only collect market fee after HARDFORK_0_5_TIME");
      _account = &d.get_account_statistics_by_uid(o.account);
      auto iter = _account->uncollected_market_fees.find(o.asset_aid);
      FC_ASSERT(iter != _account->uncollected_market_fees.end(), "there is no available asset to collect.");
      FC_ASSERT(iter->second >= o.amount, "not enough amount to collect. just ${n} available.", ("n", iter->second));
      return void_result();
   } FC_CAPTURE_AND_RETHROW((o))
}

void_result market_fee_collect_evaluator::do_apply(const market_fee_collect_operation& o)
{
   try {
      database& d = db();
      if (_account){
         d.modify(*_account, [&](account_statistics_object& s) {
            auto iter = s.uncollected_market_fees.find(o.asset_aid);
            if (iter->second == o.amount){
               s.uncollected_market_fees.erase(iter);
            }
            else{
               iter->second -= o.amount;
            }
         });
         asset ast(o.amount, o.asset_aid);
         d.adjust_balance(o.account, ast);
      }
      return void_result();
   } FC_CAPTURE_AND_RETHROW((o))
}

} } // graphene::chain
