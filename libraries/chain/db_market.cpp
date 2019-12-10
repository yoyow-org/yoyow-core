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

#include <graphene/chain/database.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/market_object.hpp>

#include <fc/uint128.hpp>

namespace graphene {
   namespace chain {
      namespace detail {
         uint64_t calculate_percent(const share_type& value, uint16_t percent)
         {
            fc::uint128 a(value.value);
            a *= percent;
            a /= GRAPHENE_100_PERCENT;
            return a.to_uint64();
         }

} //detail

/**
 * All margin positions are force closed at the swan price
 * Collateral received goes into a force-settlement fund
 * No new margin positions can be created for this asset
 * Force settlement happens without delay at the swan price, deducting from force-settlement fund
 * No more asset updates may be issued.
*/

void database::cancel_limit_order( const limit_order_object& order, bool create_virtual_op, bool skip_cancel_fee )
{
   // if need to create a virtual op, try deduct a cancellation fee here.
   // there are two scenarios when order is cancelled and need to create a virtual op:
   // 1. due to expiration: always deduct a fee if there is any fee deferred
   // 2. due to cull_small: deduct a fee after hard fork 604, but not before (will set skip_cancel_fee)
   const _account_statistics_object* seller_acc_stats = nullptr;
   const asset_dynamic_data_object* fee_asset_dyn_data = nullptr;
   limit_order_cancel_operation vop;
   if( create_virtual_op )
   {
      vop.order = order.id;
      vop.fee_paying_account = order.seller;
   }

   // refund funds in order
   auto refunded = order.amount_for_sale();
   if( refunded.asset_id == GRAPHENE_CORE_ASSET_AID )
   {
      if( seller_acc_stats == nullptr )
          seller_acc_stats = &get_account_statistics_by_uid(order.seller);
      modify(*seller_acc_stats, [&](_account_statistics_object& obj) {
         obj.total_core_in_orders -= refunded.amount;
      });
   }
   adjust_balance(order.seller, refunded);

   if( create_virtual_op )
      push_applied_operation( vop );

   remove(order);
}

bool maybe_cull_small_order( database& db, const limit_order_object& order )
{
   /**
    *  There are times when the AMOUNT_FOR_SALE * SALE_PRICE == 0 which means that we
    *  have hit the limit where the seller is asking for nothing in return.  When this
    *  happens we must refund any balance back to the seller, it is too small to be
    *  sold at the sale price.
    *
    *  If the order is a taker order (as opposed to a maker order), so the price is
    *  set by the counterparty, this check is deferred until the order becomes unmatched
    *  (see #555) -- however, detecting this condition is the responsibility of the caller.
    */
   if( order.amount_to_receive().amount == 0 )
   {
      db.cancel_limit_order( order );
      return true;
   }
   return false;
}

bool database::apply_order(const limit_order_object& new_order_object, bool allow_black_swan)
{
   auto order_id = new_order_object.id;
   asset_aid_type sell_asset_id = new_order_object.sell_asset_id();
   asset_aid_type recv_asset_id = new_order_object.receive_asset_id();

   // We only need to check if the new order will match with others if it is at the front of the book
   const auto& limit_price_idx = get_index_type<limit_order_index>().indices().get<by_price>();
   auto limit_itr = limit_price_idx.lower_bound( boost::make_tuple( new_order_object.sell_price, order_id ) );
   if( limit_itr != limit_price_idx.begin() )
   {
      --limit_itr;
      if( limit_itr->sell_asset_id() == sell_asset_id && limit_itr->receive_asset_id() == recv_asset_id )
         return false;
   }

   // this is the opposite side (on the book)
   auto max_price = ~new_order_object.sell_price;
   limit_itr = limit_price_idx.lower_bound( max_price.max() );
   auto limit_end = limit_price_idx.upper_bound( max_price );

   // Order matching should be in favor of the taker.
   // When a new limit order is created, e.g. an ask, need to check if it will match the highest bid.
   // We were checking call orders first. However, due to MSSR (maximum_short_squeeze_ratio),
   // effective price of call orders may be worse than limit orders, so we should also check limit orders here.

   // Question: will a new limit order trigger a black swan event?
   //
   // 1. as of writing, it's possible due to the call-order-and-limit-order overlapping issue:
   //       https://github.com/bitshares/bitshares-core/issues/606 .
   //    when it happens, a call order can be very big but don't match with the opposite,
   //    even when price feed is too far away, further than swan price,
   //    if the new limit order is in the same direction with the call orders, it can eat up all the opposite,
   //    then the call order will lose support and trigger a black swan event.
   // 2. after issue 606 is fixed, there will be no limit order on the opposite side "supporting" the call order,
   //    so a new order in the same direction with the call order won't trigger a black swan event.
   // 3. calling is one direction. if the new limit order is on the opposite direction,
   //    no matter if matches with the call, it won't trigger a black swan event.
   //    (if a match at MSSP caused a black swan event, it means the call order is already undercollateralized,
   //      which should trigger a black swan event earlier.)
   //
   // Since it won't trigger a black swan, no need to check here.

   // currently we don't do cross-market (triangle) matching.
   // the limit order will only match with a call order if meet all of these:
   // 1. it's buying collateral, which means sell_asset is the MIA, receive_asset is the backing asset.
   // 2. sell_asset is not a prediction market
   // 3. sell_asset is not globally settled
   // 4. sell_asset has a valid price feed
   // 5. the call order's collateral ratio is below or equals to MCR
   // 6. the limit order provided a good price


   bool finished = false; // whether the new order is gone

   // still need to check limit orders
   while (!finished && limit_itr != limit_end)
   {
      auto old_limit_itr = limit_itr;
      ++limit_itr;
      // match returns 2 when only the old order was fully filled. In this case, we keep matching; otherwise, we stop.
      finished = ( match( new_order_object, *old_limit_itr, old_limit_itr->sell_price ) != 2 );
   }

   const limit_order_object* updated_order_object = find< limit_order_object >( order_id );
   if( updated_order_object == nullptr )
      return true;

   return maybe_cull_small_order( *this, *updated_order_object );
}

/**
 *  Matches the two orders, the first parameter is taker, the second is maker.
 *
 *  @return a bit field indicating which orders were filled (and thus removed)
 *
 *  0 - no orders were matched
 *  1 - taker was filled
 *  2 - maker was filled
 *  3 - both were filled
 */
int database::match( const limit_order_object& usd, const limit_order_object& core, const price& match_price )
{
   FC_ASSERT( usd.sell_price.quote.asset_id == core.sell_price.base.asset_id );
   FC_ASSERT( usd.sell_price.base.asset_id  == core.sell_price.quote.asset_id );
   FC_ASSERT( usd.for_sale > 0 && core.for_sale > 0 );

   auto usd_for_sale = usd.amount_for_sale();
   auto core_for_sale = core.amount_for_sale();

   asset usd_pays, usd_receives, core_pays, core_receives;

   auto maint_time = get_dynamic_global_properties().next_maintenance_time;

   bool cull_taker = false;
   if( usd_for_sale <= core_for_sale * match_price ) // rounding down here should be fine
   {
      usd_receives  = usd_for_sale * match_price; // round down, in favor of bigger order

      // Be here, it's possible that taker is paying something for nothing due to partially filled in last loop.
      // In this case, we see it as filled and cancel it later
      if (usd_receives.amount == 0)
         return 1;

      core_receives = usd_receives.multiply_and_round_up(match_price);
      cull_taker = true;
   }
   else
   {
      //This line once read: assert( core_for_sale < usd_for_sale * match_price );
      //This assert is not always true -- see trade_amount_equals_zero in operation_tests.cpp
      //Although usd_for_sale is greater than core_for_sale * match_price, core_for_sale == usd_for_sale * match_price
      //Removing the assert seems to be safe -- apparently no asset is created or destroyed.

      // The maker won't be paying something for nothing, since if it would, it would have been cancelled already.
      core_receives = core_for_sale * match_price; // round down, in favor of bigger order
         // The remaining amount in order `core` would be too small,
         //   so the order will be culled in fill_limit_order() below
      usd_receives = core_receives.multiply_and_round_up( match_price );
   }

   core_pays = usd_receives;
   usd_pays  = core_receives;

   int result = 0;
   result |= fill_limit_order( usd, usd_pays, usd_receives, cull_taker, match_price, false ); // the first param is taker
   result |= fill_limit_order( core, core_pays, core_receives, true, match_price, true ) << 1; // the second param is maker
   FC_ASSERT( result != 0 );
   return result;
}

bool database::fill_limit_order( const limit_order_object& order, const asset& pays, const asset& receives, bool cull_if_small,
                           const price& fill_price, const bool is_maker )
{ try {
    cull_if_small |= false;

   FC_ASSERT( order.amount_for_sale().asset_id == pays.asset_id );
   FC_ASSERT( pays.asset_id != receives.asset_id );

   const account_object& seller = get_account_by_uid(order.seller);
   const asset_object& recv_asset = get_asset_by_aid(receives.asset_id);

   auto issuer_fees = pay_market_fees(seller, recv_asset, receives);
   pay_order( seller, receives - issuer_fees, pays );

   assert( pays.asset_id != receives.asset_id );
   push_applied_operation( fill_order_operation( order.id, order.seller, pays, receives, issuer_fees, fill_price, is_maker ) );

   if( pays == order.amount_for_sale() )
   {
      remove( order );
      return true;
   }
   else
   {
      modify( order, [&]( limit_order_object& b ) {
                             b.for_sale -= pays.amount;
                          });
      if( cull_if_small )
         return maybe_cull_small_order( *this, order );
      return false;
   }
} FC_CAPTURE_AND_RETHROW( (order)(pays)(receives) ) }


void database::pay_order( const account_object& receiver, const asset& receives, const asset& pays )
{
   const auto& balances = receiver.statistics(*this);
   if (pays.asset_id == GRAPHENE_CORE_ASSET_AID)
   {
      modify(balances, [&](_account_statistics_object& b){
         b.total_core_in_orders -= pays.amount;
      });

      //total_core_in_orders changed and update votes
      auto update_seller_votes = [&](account_uid_type account){
         const _account_statistics_object& account_stats = get_account_statistics_by_uid(account);
         if (account_stats.is_voter)
         {
            const voter_object* voter = find_voter(account, account_stats.last_voter_sequence);
            //refresh effective votes
            update_voter_effective_votes(*voter);
            //update votes
            modify(*voter, [&](voter_object& v)
            {
               v.votes = account_stats.get_votes_from_core_balance();
               v.votes_last_update = head_block_time();
            });
            //refresh effective votes again
            update_voter_effective_votes(*voter);
         }
      };
      update_seller_votes(receiver.uid);
   }
   adjust_balance(receiver.uid, receives);
}

asset database::calculate_market_fee(const asset_object& trade_asset, const asset& trade_amount)
{
   assert( trade_asset.asset_id == trade_amount.asset_id );

   if( !trade_asset.charges_market_fees() )
      return trade_asset.amount(0);
   if( trade_asset.options.market_fee_percent == 0 )
      return trade_asset.amount(0);

   fc::uint128 a(trade_amount.amount.value);
   a *= trade_asset.options.market_fee_percent;
   a /= GRAPHENE_100_PERCENT;
   asset percent_fee = trade_asset.amount(a.to_uint64());

   if( percent_fee.amount > trade_asset.options.max_market_fee )
      percent_fee.amount = trade_asset.options.max_market_fee;

   return percent_fee;
}

asset database::pay_market_fees( const asset_object& recv_asset, const asset& receives )
{
   auto issuer_fees = calculate_market_fee( recv_asset, receives );
   assert(issuer_fees <= receives );

   //Don't dirty undo state if not actually collecting any fees
   if( issuer_fees.amount > 0 )
   {
      const auto& recv_dyn_data = recv_asset.dynamic_asset_data_id(*this);
      modify( recv_dyn_data, [&]( asset_dynamic_data_object& obj ){
                   //idump((issuer_fees));
         obj.accumulated_fees += issuer_fees.amount;
      });
   }

   return issuer_fees;
}

asset database::pay_market_fees(const account_object& seller, const asset_object& recv_asset, const asset& receives)
{
   const auto issuer_fees = calculate_market_fee(recv_asset, receives);
   FC_ASSERT(issuer_fees <= receives, "Market fee shouldn't be greater than receives");
   //Don't dirty undo state if not actually collecting any fees
   if (issuer_fees.amount > 0)
   {
      asset reward = recv_asset.amount(0);
      if (recv_asset.options.extensions.valid()){
         const auto reward_percent = recv_asset.options.extensions->value.reward_percent;
         if (reward_percent && *reward_percent){
            const auto reward_value = detail::calculate_percent(issuer_fees.amount, *reward_percent);
            if (reward_value > 0){
               reward = recv_asset.amount(reward_value);
               FC_ASSERT(reward < issuer_fees, "Market reward should be less than issuer fees");

               // calculate and pay rewards
               const auto referrer_rewards_percentage = seller.reg_info.referrer_percent;
               const auto registrar_rewards_percentage = seller.reg_info.registrar_percent;
               FC_ASSERT(referrer_rewards_percentage + registrar_rewards_percentage == GRAPHENE_100_PERCENT);
               const _account_statistics_object& referrer_account = get_account_statistics_by_uid(seller.reg_info.referrer);
               const _account_statistics_object& registrar_account = get_account_statistics_by_uid(seller.reg_info.registrar);

               share_type referrer_rewards_value = detail::calculate_percent(reward.amount, referrer_rewards_percentage);
               modify(referrer_account, [&](_account_statistics_object& obj){
                  obj.add_uncollected_market_fee(recv_asset.asset_id, referrer_rewards_value);
               });
               share_type registrar_reward_value = reward.amount - referrer_rewards_value;
               modify(registrar_account, [&](_account_statistics_object& obj){
                  obj.add_uncollected_market_fee(recv_asset.asset_id, registrar_reward_value);
               });
            }
         }
      }

      const auto& recv_dyn_data = recv_asset.dynamic_asset_data_id(*this);
      share_type accumulated_fee = issuer_fees.amount - reward.amount;
      if (accumulated_fee > 0)
         modify(recv_dyn_data, [&](asset_dynamic_data_object& obj){
         obj.accumulated_fees += accumulated_fee;
         });
   }
   return issuer_fees;
}


} }
