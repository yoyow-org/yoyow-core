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
#include <graphene/chain/vesting_balance_object.hpp>
#include <graphene/chain/witness_object.hpp>

namespace graphene { namespace chain {

asset database::get_balance(account_uid_type owner, asset_aid_type asset_id) const
{
   auto& index = get_index_type<account_balance_index>().indices().get<by_account_asset>();
   auto itr = index.find(boost::make_tuple(owner, asset_id));
   if( itr == index.end() )
      return asset(0, asset_id);
   return itr->get_balance();
}

asset database::get_balance(const account_object& owner, const asset_object& asset_obj) const
{
   return get_balance(owner.get_uid(), object_id_type(asset_obj.get_id()).instance());
}

asset database::get_balance(const account_object& owner, asset_aid_type asset_id) const
{
   return get_balance(owner.get_uid(), asset_id);
}

string database::to_pretty_string( const asset& a )const
{
   const auto& ao = get_asset_by_aid( a.asset_id );
   return ao.amount_to_pretty_string( a.amount );
}

string database::to_pretty_core_string( const share_type amount )const
{
   const auto& ao = get_asset_by_aid( GRAPHENE_CORE_ASSET_AID );
   return ao.amount_to_pretty_string( amount );
}

void database::adjust_balance(const account_object& account, asset delta )
{
   adjust_balance( account.uid, delta );
}

void database::adjust_balance(account_uid_type account, asset delta )
{ try {
   if( delta.amount == 0 )
      return;

   auto& index = get_index_type<account_balance_index>().indices().get<by_account_asset>();
   auto itr = index.find(boost::make_tuple(account, delta.asset_id));
   if(itr == index.end())
   {
      FC_ASSERT( delta.amount > 0, "Insufficient Balance: account ${a}'s balance of ${b} is less than required ${r}",
                 ("a",account)
                 ("b",to_pretty_string(asset(0,delta.asset_id)))
                 ("r",to_pretty_string(-delta)) );
      create<account_balance_object>([account,&delta](account_balance_object& b) {
         b.owner = account;
         b.asset_type = delta.asset_id;
         b.balance = delta.amount.value;
      });
   } else {
      if( delta.amount < 0 )
         FC_ASSERT( itr->get_balance() >= -delta,
                    "Insufficient Balance: account ${a}'s balance of ${b} is less than required ${r}",
                    ("a",account)
                    ("b",to_pretty_string(itr->get_balance()))
                    ("r",to_pretty_string(-delta)) );
      modify(*itr, [delta](account_balance_object& b) {
         b.adjust_balance(delta);
      });
   }
   // Update coin_seconds_earned and etc
   if( delta.asset_id == GRAPHENE_CORE_ASSET_AID )
   {
      const account_statistics_object& account_stats = get_account_statistics_by_uid( account );
      if( delta.amount < 0 )
      {
         auto available_balance = account_stats.core_balance
                                - account_stats.core_leased_out
                                - account_stats.total_witness_pledge
                                - account_stats.total_platform_pledge
                                - account_stats.total_committee_member_pledge;
         FC_ASSERT( available_balance >= -delta.amount,
                    "Insufficient Balance: account ${a}'s available balance of ${b} is less than required ${r}",
                    ("a",account)
                    ("b",to_pretty_core_string(available_balance))
                    ("r",to_pretty_string(-delta)) );
         const auto& global_params = get_global_properties().parameters;
         if( account_stats.is_voter && account_stats.core_balance + delta.amount < global_params.min_governance_voting_balance )
         {
            const voter_object* voter = find_voter( account, account_stats.last_voter_sequence );
            invalidate_voter( *voter );
         }
      }
      if( account_stats.is_voter )
      {
         const voter_object* voter = find_voter( account, account_stats.last_voter_sequence );
         //refresh effective votes
         update_voter_effective_votes( *voter );
         //update votes
         modify( *voter, [&]( voter_object& v )
         {
            v.votes += delta.amount.value;
            v.votes_last_update = head_block_time();
         } );
         //refresh effective votes again
         update_voter_effective_votes( *voter );
      }
      const uint64_t csaf_window = get_global_properties().parameters.csaf_accumulate_window;
      modify( account_stats, [&](account_statistics_object& s) {
         s.update_coin_seconds_earned( csaf_window, head_block_time() );
         s.core_balance += delta.amount;
      });
   }

} FC_CAPTURE_AND_RETHROW( (account)(delta) ) }

optional< vesting_balance_id_type > database::deposit_lazy_vesting(
   const optional< vesting_balance_id_type >& ovbid,
   share_type amount, uint32_t req_vesting_seconds,
   account_uid_type req_owner,
   bool require_vesting )
{
   if( amount == 0 )
      return optional< vesting_balance_id_type >();

   fc::time_point_sec now = head_block_time();

   while( true )
   {
      if( !ovbid.valid() )
         break;
      const vesting_balance_object& vbo = (*ovbid)(*this);
      if( vbo.owner != req_owner )
         break;
      if( vbo.policy.which() != vesting_policy::tag< cdd_vesting_policy >::value )
         break;
      if( vbo.policy.get< cdd_vesting_policy >().vesting_seconds != req_vesting_seconds )
         break;
      modify( vbo, [&]( vesting_balance_object& _vbo )
      {
         if( require_vesting )
            _vbo.deposit(now, amount);
         else
            _vbo.deposit_vested(now, amount);
      } );
      return optional< vesting_balance_id_type >();
   }

   const vesting_balance_object& vbo = create< vesting_balance_object >( [&]( vesting_balance_object& _vbo )
   {
      _vbo.owner = req_owner;
      _vbo.balance = amount;

      cdd_vesting_policy policy;
      policy.vesting_seconds = req_vesting_seconds;
      policy.coin_seconds_earned = require_vesting ? 0 : amount.value * policy.vesting_seconds;
      policy.coin_seconds_earned_last_update = now;

      _vbo.policy = policy;
   } );

   return vbo.id;
}

void database::deposit_cashback(const account_object& acct, share_type amount, bool require_vesting)
{
   // If we don't have a VBO, or if it has the wrong maturity
   // due to a policy change, cut it loose.

   if( amount == 0 )
      return;

   if( acct.get_id() == GRAPHENE_COMMITTEE_ACCOUNT || acct.get_id() == GRAPHENE_WITNESS_ACCOUNT ||
       acct.get_id() == GRAPHENE_RELAXED_COMMITTEE_ACCOUNT || acct.get_id() == GRAPHENE_NULL_ACCOUNT ||
       acct.get_id() == GRAPHENE_TEMP_ACCOUNT )
   {
      // The blockchain's accounts do not get cashback; it simply goes to the reserve pool.
      modify(get_core_asset().dynamic_asset_data_id(*this), [amount](asset_dynamic_data_object& d) {
         d.current_supply -= amount;
      });
      return;
   }

   optional< vesting_balance_id_type > new_vbid = deposit_lazy_vesting(
      acct.cashback_vb,
      amount,
      get_global_properties().parameters.cashback_vesting_period_seconds,
      acct.uid,
      require_vesting );

   if( new_vbid.valid() )
   {
      modify( acct, [&]( account_object& _acct )
      {
         _acct.cashback_vb = *new_vbid;
      } );
   }

   return;
}

void database::deposit_witness_pay(const witness_object& wit, share_type amount)
{
   FC_ASSERT( amount >= 0 );

   if( amount == 0 )
      return;

   const auto& account_stats = get_account_statistics_by_uid( wit.account );
   modify( account_stats, [&](account_statistics_object& s) {
      s.uncollected_witness_pay += amount;
   } );

   return;
}

} }
