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
#include <graphene/chain/database.hpp>
#include <graphene/chain/hardfork.hpp>
#include <fc/uint128.hpp>

namespace graphene { namespace chain {

share_type cut_fee(share_type a, uint16_t p)
{
   if( a == 0 || p == 0 )
      return 0;
   if( p == GRAPHENE_100_PERCENT )
      return a;

   fc::uint128 r(a.value);
   r *= p;
   r /= GRAPHENE_100_PERCENT;
   return r.to_uint64();
}

void account_balance_object::adjust_balance(const asset& delta)
{
   assert(delta.asset_id == asset_type);
   balance += delta.amount;
}

std::pair<fc::uint128_t, share_type> _account_statistics_object::compute_coin_seconds_earned_fix(const uint64_t window, const fc::time_point_sec now, const database& db, uint8_t enable_hard_fork_type)const
{
   if (enable_hard_fork_type < ENABLE_HEAD_FORK_05)
      enable_hard_fork_type = ENABLE_HEAD_FORK_NONE;
   return compute_coin_seconds_earned(window,now,db, enable_hard_fork_type);
}


std::pair<fc::uint128_t, share_type> _account_statistics_object::compute_coin_seconds_earned(const uint64_t window, const fc::time_point_sec now, const database& db, const uint8_t enable_hard_fork_type)const
{
   fc::time_point_sec now_rounded((now.sec_since_epoch() / 60) * 60);
   // check average coins and max coin-seconds
   share_type new_average_coins;
   fc::uint128_t max_coin_seconds;
   share_type effective_balance;
   switch (enable_hard_fork_type){
   case ENABLE_HEAD_FORK_NONE :
      effective_balance = core_balance + core_leased_in - core_leased_out;
      break;
   case ENABLE_HEAD_FORK_04 :
      if (pledge_balance_ids.count(pledge_balance_type::Witness)) {
         effective_balance = core_balance + core_leased_in - core_leased_out -
            get_pledge_balance(GRAPHENE_CORE_ASSET_AID, pledge_balance_type::Witness, db);
      } else
         effective_balance = core_balance + core_leased_in - core_leased_out;
      break;
   case ENABLE_HEAD_FORK_05 :
      if (pledge_balance_ids.count(pledge_balance_type::Lock_balance)) {
         auto pledge_balance_obj = pledge_balance_ids.at(pledge_balance_type::Lock_balance)(db);
         if (GRAPHENE_CORE_ASSET_AID == pledge_balance_obj.asset_id)
            effective_balance = pledge_balance_obj.pledge;
      } 
      break;
   default:
      break;
   }

   if (now_rounded <= average_coins_last_update)
      new_average_coins = average_coins;
   else
   {
      uint64_t delta_seconds = (now_rounded - average_coins_last_update).to_seconds();
      if (delta_seconds >= window)
         new_average_coins = effective_balance;
      else
      {
         uint64_t old_seconds = window - delta_seconds;

         fc::uint128_t old_coin_seconds = fc::uint128_t(average_coins.value) * old_seconds;
         fc::uint128_t new_coin_seconds = fc::uint128_t(effective_balance.value) * delta_seconds;

         max_coin_seconds = old_coin_seconds + new_coin_seconds;
         new_average_coins = (max_coin_seconds / window).to_uint64();
      }
   }
   // kill rounding issue
   max_coin_seconds = fc::uint128_t(new_average_coins.value) * window;

   // check earned coin-seconds
   fc::uint128_t new_coin_seconds_earned;
   if (now_rounded <= coin_seconds_earned_last_update)
      new_coin_seconds_earned = coin_seconds_earned;
   else
   {
      int64_t delta_seconds = (now_rounded - coin_seconds_earned_last_update).to_seconds();

      fc::uint128_t delta_coin_seconds = effective_balance.value;
      delta_coin_seconds *= delta_seconds;

      new_coin_seconds_earned = coin_seconds_earned + delta_coin_seconds;
   }
   if (new_coin_seconds_earned > max_coin_seconds)
      new_coin_seconds_earned = max_coin_seconds;

   return std::make_pair(new_coin_seconds_earned, new_average_coins);
}

void _account_statistics_object::update_coin_seconds_earned(const uint64_t window, const fc::time_point_sec now, const database& db, const uint8_t enable_hard_fork_type)
{
   fc::time_point_sec now_rounded( ( now.sec_since_epoch() / 60 ) * 60 );
   if( now_rounded <= coin_seconds_earned_last_update && now_rounded <= average_coins_last_update )
      return;
   const auto& result = compute_coin_seconds_earned( window, now_rounded , db, enable_hard_fork_type);
   coin_seconds_earned = result.first;
   coin_seconds_earned_last_update = now_rounded;
   average_coins = result.second;
   average_coins_last_update = now_rounded;
}

void _account_statistics_object::set_coin_seconds_earned(const fc::uint128_t new_coin_seconds, const fc::time_point_sec now)
{
   fc::time_point_sec now_rounded( ( now.sec_since_epoch() / 60 ) * 60 );
   coin_seconds_earned = new_coin_seconds;
   if( coin_seconds_earned_last_update < now_rounded )
      coin_seconds_earned_last_update = now_rounded;
}

void _account_statistics_object::add_uncollected_market_fee(asset_aid_type asset_aid, share_type amount)
{
   auto iter = uncollected_market_fees.find(asset_aid);
   if (iter != uncollected_market_fees.end()){
      iter->second += amount;
   }
   else{
      uncollected_market_fees.insert(std::make_pair(asset_aid, amount));
   }
}

set<account_uid_type> account_member_index::get_account_members(const account_object& a)const
{
   set<account_uid_type> result;
   for( auto auth : a.owner.account_uid_auths )
      result.insert(auth.first.uid);
   for( auto auth : a.active.account_uid_auths )
      result.insert(auth.first.uid);
   for( auto auth : a.secondary.account_uid_auths )
      result.insert(auth.first.uid);
   return result;
}
set<public_key_type> account_member_index::get_key_members(const account_object& a)const
{
   set<public_key_type> result;
   for( auto auth : a.owner.key_auths )
      result.insert(auth.first);
   for( auto auth : a.active.key_auths )
      result.insert(auth.first);
   result.insert( a.memo_key );
   return result;
}

void account_member_index::object_inserted(const object& obj)
{
    assert( dynamic_cast<const account_object*>(&obj) ); // for debug only
    const account_object& a = static_cast<const account_object&>(obj);

    auto account_members = get_account_members(a);
    for( auto item : account_members )
       account_to_account_memberships[item].insert(a.uid);

    auto key_members = get_key_members(a);
    for( auto item : key_members )
       account_to_key_memberships[item].insert(a.uid);
}

void account_member_index::object_removed(const object& obj)
{
    assert( dynamic_cast<const account_object*>(&obj) ); // for debug only
    const account_object& a = static_cast<const account_object&>(obj);

    auto key_members = get_key_members(a);
    for( auto item : key_members )
       account_to_key_memberships[item].erase( a.uid );

    auto account_members = get_account_members(a);
    for( auto item : account_members )
       account_to_account_memberships[item].erase( a.uid );
}

void account_member_index::about_to_modify(const object& before)
{
   before_key_members.clear();
   before_account_members.clear();
   assert( dynamic_cast<const account_object*>(&before) ); // for debug only
   const account_object& a = static_cast<const account_object&>(before);
   before_key_members     = get_key_members(a);
   before_account_members = get_account_members(a);
}

void account_member_index::object_modified(const object& after)
{
    assert( dynamic_cast<const account_object*>(&after) ); // for debug only
    const account_object& a = static_cast<const account_object&>(after);

    {
       set<account_uid_type> after_account_members = get_account_members(a);
       vector<account_uid_type> removed; removed.reserve(before_account_members.size());
       std::set_difference(before_account_members.begin(), before_account_members.end(),
                           after_account_members.begin(), after_account_members.end(),
                           std::inserter(removed, removed.end()));

       for( auto itr = removed.begin(); itr != removed.end(); ++itr )
          account_to_account_memberships[*itr].erase(a.uid);

       vector<account_uid_type> added; added.reserve(after_account_members.size());
       std::set_difference(after_account_members.begin(), after_account_members.end(),
                           before_account_members.begin(), before_account_members.end(),
                           std::inserter(added, added.end()));

       for( auto itr = added.begin(); itr != added.end(); ++itr )
          account_to_account_memberships[*itr].insert(a.uid);
    }


    {
       set<public_key_type> after_key_members = get_key_members(a);

       vector<public_key_type> removed; removed.reserve(before_key_members.size());
       std::set_difference(before_key_members.begin(), before_key_members.end(),
                           after_key_members.begin(), after_key_members.end(),
                           std::inserter(removed, removed.end()));

       for( auto itr = removed.begin(); itr != removed.end(); ++itr )
          account_to_key_memberships[*itr].erase(a.uid);

       vector<public_key_type> added; added.reserve(after_key_members.size());
       std::set_difference(after_key_members.begin(), after_key_members.end(),
                           before_key_members.begin(), before_key_members.end(),
                           std::inserter(added, added.end()));

       for( auto itr = added.begin(); itr != added.end(); ++itr )
          account_to_key_memberships[*itr].insert(a.uid);
    }
}

void account_referrer_index::object_inserted( const object& obj )
{
}
void account_referrer_index::object_removed( const object& obj )
{
}
void account_referrer_index::about_to_modify( const object& before )
{
}
void account_referrer_index::object_modified( const object& after  )
{
}

} } // graphene::chain
