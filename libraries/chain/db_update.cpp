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
#include <graphene/chain/db_with.hpp>

#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/global_property_object.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/proposal_object.hpp>
#include <graphene/chain/transaction_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/advertising_object.hpp>

#include <graphene/chain/protocol/fee_schedule.hpp>

#include <fc/uint128.hpp>
#include <boost/multiprecision/cpp_int.hpp>

namespace graphene { namespace chain {

void database::update_global_dynamic_data( const signed_block& b )
{
   const global_property_object& gpo = get_global_properties();
   const dynamic_global_property_object& _dgp =
      dynamic_global_property_id_type(0)(*this);

   uint32_t missed_blocks = get_slot_at_time( b.timestamp );
   assert( missed_blocks != 0 );
   missed_blocks--;
   
   //skip miss block when uint test
   if(!(get_node_properties().skip_flags&skip_uint_test))
      for( uint32_t i = 0; i < missed_blocks; ++i ) {
         const auto& witness_missed = get_witness_by_uid( get_scheduled_witness( i+1 ) );
         if(  witness_missed.account != b.witness ) {
            /*
            const auto& witness_account = witness_missed.account(*this);
            if( (fc::time_point::now() - b.timestamp) < fc::seconds(30) )
               wlog( "Witness ${name} missed block ${n} around ${t}", ("name",witness_account.name)("n",b.block_num())("t",b.timestamp) );
               */

            modify( witness_missed, [&]( witness_object& w ) {
              w.total_missed++;
              if( w.last_confirmed_block_num + gpo.parameters.max_witness_inactive_blocks < b.block_num() )
                 w.signing_key = public_key_type();
            });
            modify(get_account_statistics_by_uid(witness_missed.account), [&](_account_statistics_object& s) {
              s.witness_total_missed++;
            });
         }
      }

   // dynamic global properties updating
   modify( _dgp, [&]( dynamic_global_property_object& dgp ){
      if( BOOST_UNLIKELY( b.block_num() == 1 ) )
         dgp.recently_missed_count = 0;
         else if( _checkpoints.size() && _checkpoints.rbegin()->first >= b.block_num() )
         dgp.recently_missed_count = 0;
      else if( missed_blocks )
         dgp.recently_missed_count += GRAPHENE_RECENTLY_MISSED_COUNT_INCREMENT*missed_blocks;
      else if( dgp.recently_missed_count > GRAPHENE_RECENTLY_MISSED_COUNT_INCREMENT )
         dgp.recently_missed_count -= GRAPHENE_RECENTLY_MISSED_COUNT_DECREMENT;
      else if( dgp.recently_missed_count > 0 )
         dgp.recently_missed_count--;

      dgp.head_block_number = b.block_num();
      dgp.head_block_id = b.id();
      dgp.time = b.timestamp;
      dgp.current_witness = b.witness;
      dgp.recent_slots_filled = (
           (dgp.recent_slots_filled << 1)
           + 1) << missed_blocks;
      dgp.current_aslot += missed_blocks+1;
   });

   if( !(get_node_properties().skip_flags & skip_undo_history_check) )
   {
      GRAPHENE_ASSERT( _dgp.head_block_number - _dgp.last_irreversible_block_num  < GRAPHENE_MAX_UNDO_HISTORY, undo_database_exception,
                 "The database does not have enough undo history to support a blockchain with so many missed blocks. "
                 "Please add a checkpoint if you would like to continue applying blocks beyond this point.",
                 ("last_irreversible_block_num",_dgp.last_irreversible_block_num)("head", _dgp.head_block_number)
                 ("recently_missed",_dgp.recently_missed_count)("max_undo",GRAPHENE_MAX_UNDO_HISTORY) );
   }
}

void database::update_undo_db_size()
{
   const dynamic_global_property_object& _dgp = dynamic_global_property_id_type(0)(*this);

   _undo_db.set_max_size( _dgp.head_block_number - _dgp.last_irreversible_block_num + 1 );
   _fork_db.set_max_size( _dgp.head_block_number - _dgp.last_irreversible_block_num + 1 );
}

void database::update_signing_witness(const witness_object& signing_witness, const signed_block& new_block)
{
   const global_property_object& gpo = get_global_properties();
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   uint64_t new_block_aslot = dpo.current_aslot + get_slot_at_time( new_block.timestamp );

   const auto& itr = gpo.active_witnesses.find( signing_witness.account );
   FC_ASSERT( itr != gpo.active_witnesses.end() );
   auto wit_type = itr->second;

   const auto& core_asset = get_core_asset();
   share_type budget_this_block = std::min( dpo.total_budget_per_block, core_asset.reserved( *this ) );

   share_type witness_pay;
   if( wit_type == scheduled_by_vote_top )
      witness_pay = gpo.parameters.by_vote_top_witness_pay_per_block;
   else if (wit_type == scheduled_by_vote_rest)
      witness_pay = gpo.parameters.by_vote_rest_witness_pay_per_block;
   else if (wit_type == scheduled_by_pledge)
      witness_pay = dpo.by_pledge_witness_pay_per_block;
   witness_pay = std::min( witness_pay, budget_this_block );
   
   share_type budget_remained = budget_this_block - witness_pay;
   FC_ASSERT( budget_remained >= 0 );

   if( budget_this_block > 0 )
   {
      const auto& core_dyn_data = core_asset.dynamic_data(*this);
      modify( core_dyn_data, [&]( asset_dynamic_data_object& dyn )
      {
         dyn.current_supply += budget_this_block;
      } );
   }

   if( budget_remained > 0 )
   {
      modify( dpo, [&]( dynamic_global_property_object& _dpo )
      {
         _dpo.budget_pool += budget_remained;
      } );
   }

   if( witness_pay > 0 )
      deposit_witness_pay( signing_witness, witness_pay, wit_type );

   modify( signing_witness, [&]( witness_object& _wit )
   {
      _wit.last_aslot = new_block_aslot;
      _wit.total_produced += 1;
      _wit.last_confirmed_block_num = new_block.block_num();
   } );

   modify(get_account_statistics_by_uid(signing_witness.account), [&](_account_statistics_object& _stat)
   {
      _stat.witness_last_aslot = new_block_aslot;
      _stat.witness_total_produced += 1;
      _stat.witness_last_confirmed_block_num = new_block.block_num();
   } );
}

share_type database::get_witness_pay_by_pledge(const global_property_object& gpo, const dynamic_global_property_object& dpo, const uint16_t by_pledge_witness_count)
{
   if (head_block_time() < HARDFORK_0_4_TIME)
      return gpo.parameters.by_pledge_witness_pay_per_block;

   const uint64_t witness_pay_first_modulus    = 1052;
   const uint64_t witness_pay_second_modulus   = 69370;
   const uint64_t witness_pay_third_modulus    = 1656000;
   const uint64_t witness_pay_four_modulus     = 21120000;
   const uint64_t witness_pay_percent          = 1000000;
   const uint64_t witness_pay_lower_point      = GRAPHENE_BLOCKCHAIN_PRECISION * uint64_t(10000000);
   const uint64_t witness_pay_upper_point      = GRAPHENE_BLOCKCHAIN_PRECISION * uint64_t(320000000);
   const uint64_t witness_pay_lower_point_rate = GRAPHENE_1_PERCENT * 25;
   share_type total_witness_pledges = dpo.total_witness_pledge;
   bigint witness_pay_per_year = 0;
   if (total_witness_pledges < witness_pay_lower_point) {
      witness_pay_per_year = (bigint)witness_pay_lower_point_rate * total_witness_pledges.value / GRAPHENE_100_PERCENT;
   }
   else if (total_witness_pledges < witness_pay_upper_point) {
      bigint pledge = total_witness_pledges.value;
      bigint A = GRAPHENE_BLOCKCHAIN_PRECISION * 10000000;

      /*
      * when total witness pledge between 10 million and 320 million, witness_pay_per_year is calculated as follows:
      * rate = (-0.001052*pledge*pledge*pledge + 0.06937*pledge*pledge - 1.656*pledge + 21.12)/100, pledge unit is 10 million;
      * witness_pay_per_year = pledge * rate,
      */
      bigint rate = pledge * pledge * witness_pay_second_modulus * A
         - pledge * pledge * pledge * witness_pay_first_modulus
         - pledge * witness_pay_third_modulus * A * A
         + (bigint)witness_pay_four_modulus * A * A * A;

      witness_pay_per_year = pledge * rate * GRAPHENE_1_PERCENT /
         (A*A*A*witness_pay_percent*GRAPHENE_100_PERCENT);
   }
   else {
      witness_pay_per_year = 150110208 * GRAPHENE_BLOCKCHAIN_PRECISION / 10;
   }

   share_type witness_pay = (witness_pay_per_year * gpo.parameters.block_interval *gpo.active_witnesses.size() 
      / (86400 * 365 * by_pledge_witness_count)).to_int64();

   return witness_pay;
}

void database::update_last_irreversible_block()
{
   const global_property_object& gpo = get_global_properties();
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();

   vector< const witness_object* > wit_objs;
   wit_objs.reserve( gpo.active_witnesses.size() );
   for( const auto& wid : gpo.active_witnesses )
      wit_objs.push_back( &(get_witness_by_uid( wid.first )) );

   static_assert( GRAPHENE_IRREVERSIBLE_THRESHOLD > 0, "irreversible threshold must be nonzero" );

   // 1 1 1 2 2 2 2 2 2 2 -> 2     .7*10 = 7
   // 1 1 1 1 1 1 1 2 2 2 -> 1
   // 3 3 3 3 3 3 3 3 3 3 -> 3

   size_t offset = ((GRAPHENE_100_PERCENT - GRAPHENE_IRREVERSIBLE_THRESHOLD) * wit_objs.size() / GRAPHENE_100_PERCENT);

   std::nth_element( wit_objs.begin(), wit_objs.begin() + offset, wit_objs.end(),
      []( const witness_object* a, const witness_object* b )
      {
         return a->last_confirmed_block_num < b->last_confirmed_block_num;
      } );

   uint32_t new_last_irreversible_block_num = wit_objs[offset]->last_confirmed_block_num;

   if( new_last_irreversible_block_num > dpo.last_irreversible_block_num )
   {
      modify( dpo, [&]( dynamic_global_property_object& _dpo )
      {
         _dpo.last_irreversible_block_num = new_last_irreversible_block_num;
      } );
   }
}

void database::clear_expired_transactions()
{ try {
   //Look for expired transactions in the deduplication list, and remove them.
   //Transactions must have expired by at least two forking windows in order to be removed.
   auto& transaction_idx = static_cast<transaction_index&>(get_mutable_index(implementation_ids, impl_transaction_object_type));
   const auto& dedupe_index = transaction_idx.indices().get<by_expiration>();
   while( (!dedupe_index.empty()) && (head_block_time() > dedupe_index.begin()->trx.expiration) )
      transaction_idx.remove(*dedupe_index.begin());
} FC_CAPTURE_AND_RETHROW() }

void database::clear_expired_proposals()
{
   const auto& proposal_expiration_index = get_index_type<proposal_index>().indices().get<by_expiration>();
   while( !proposal_expiration_index.empty() && proposal_expiration_index.begin()->expiration_time <= head_block_time() )
   {
      const proposal_object& proposal = *proposal_expiration_index.begin();
      processed_transaction result;
      try {
         auto value = proposal.is_authorized_to_execute(*this);
         if (value.first)
         {
            result = push_proposal(proposal, value.second);
            //TODO: Do something with result so plugins can process it.
            continue;
         }
      } catch( const fc::exception& e ) {
         elog("Failed to apply proposed transaction on its expiration. Deleting it.\n${proposal}\n${error}",
              ("proposal", proposal)("error", e.to_detail_string()));
      }
      remove(proposal);
   }
}

void database::clear_active_post()
{
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   if (dpo.current_active_post_sequence <= _latest_active_post_periods)
      return;

   const auto& apt_idx = get_index_type<active_post_index>().indices().get<by_period_sequence>();
   const auto& apt_end = apt_idx.lower_bound(dpo.current_active_post_sequence - _latest_active_post_periods + 1);
   auto apt_itr = apt_idx.begin();
   while (apt_itr != apt_end)
   {
      remove(*apt_itr);
      apt_itr = apt_idx.begin();
   }
}

void database::clear_unnecessary_objects()
{
   const auto block_time = head_block_time();
   switch (head_block_num() % 10) {
   case 0: 
   {
      if (block_time < time_point_sec(_advertising_order_remaining_time))
         break;
      const auto& ado_idx = get_index_type<advertising_order_index>().indices().get<by_clear_time>();
      const auto& ado_end = ado_idx.lower_bound(block_time - _advertising_order_remaining_time);
      auto ado_itr = ado_idx.begin();

      while (ado_itr != ado_end) {
         remove(*ado_itr);
         ado_itr = ado_idx.begin();
      }
      break;
   }
   case 3:
   {
      if (block_time < time_point_sec(_custom_vote_remaining_time))
         break;
      const auto& custom_vote_idx = get_index_type<custom_vote_index>().indices().get<by_expired_time>();
      const auto& custom_vote_end = custom_vote_idx.lower_bound(block_time - _custom_vote_remaining_time);
      auto custom_vote_itr = custom_vote_idx.begin();

      while (custom_vote_itr != custom_vote_end) {
         const auto& cast_vote_idx = get_index_type<cast_custom_vote_index>().indices().get<by_custom_vote_vid>();
         auto cast_vote_itr = cast_vote_idx.lower_bound(std::make_tuple(custom_vote_itr->custom_vote_creator, custom_vote_itr->vote_vid));

         while (cast_vote_itr != cast_vote_idx.end() && cast_vote_itr->custom_vote_creator == custom_vote_itr->custom_vote_creator &&
            cast_vote_itr->custom_vote_vid == custom_vote_itr->vote_vid) {
            auto del = cast_vote_itr;
            ++cast_vote_itr;
            remove(*del);    
         } 

         remove(*custom_vote_itr);
         custom_vote_itr = custom_vote_idx.begin();
      }
      break;
   }
   default:
      break;
   }

}

void database::update_reduce_witness_csaf()
{
    const uint64_t csaf_window = get_global_properties().parameters.csaf_accumulate_window;
    const auto& witness_idx = get_index_type<witness_index>().indices();
    for (auto itr = witness_idx.begin(); itr != witness_idx.end(); ++itr)
    {
        const _account_statistics_object& statistics_obj = get_account_statistics_by_uid(itr->account);
        modify(statistics_obj, [&](_account_statistics_object& s) {
            s.update_coin_seconds_earned(csaf_window, head_block_time(), *this, ENABLE_HEAD_FORK_NONE);
        });
    }
}

void database::update_account_permission()
{
   const auto& account_idx = get_index_type<account_index>().indices();
   for (auto itr = account_idx.begin(); itr != account_idx.end(); ++itr)
   {
      modify(*itr, [&](account_object& a) {
         a.can_reply = true;
         a.can_rate = true;
      });
   }
}

void database::update_account_reg_info()
{
   const auto& account_idx = get_index_type<account_index>().indices();
   for (auto itr = account_idx.begin(); itr != account_idx.end(); ++itr)
   {
      modify(*itr, [&](account_object& a) {
         if (a.reg_info.registrar == GRAPHENE_NULL_ACCOUNT_UID)
            a.reg_info.registrar = account_uid_type(224373708);
         if (a.reg_info.referrer == GRAPHENE_NULL_ACCOUNT_UID)
            a.reg_info.referrer = account_uid_type(23080);
         a.reg_info.registrar_percent = GRAPHENE_100_PERCENT / 2;
         a.reg_info.referrer_percent = GRAPHENE_100_PERCENT / 2;
      });
   }
}

void database::update_core_asset_flags()
{
   auto const & core_asset = get_asset_by_aid(GRAPHENE_CORE_ASSET_AID);
   modify(core_asset, [&](asset_object& ast) {
      ast.options.flags |= charge_market_fee;
   });
}

void database::update_account_feepoint()
{
   const uint64_t csaf_window = get_global_properties().parameters.csaf_accumulate_window;
   const auto& account_idx = get_index_type<account_statistics_index>().indices();
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   for (auto itr = account_idx.begin(); itr != account_idx.end(); ++itr)
   {
      modify(*itr, [&](_account_statistics_object& s) {
         s.update_coin_seconds_earned(csaf_window, head_block_time(), *this, ENABLE_HEAD_FORK_04);
      });
   }
}

std::tuple<set<std::tuple<score_id_type, share_type, bool>>, share_type>
database::get_effective_csaf(const active_post_object& active_post)
{
   const global_property_object& gpo = get_global_properties();
   const auto& params = gpo.parameters.get_extension_params();

   uint128_t amount = (uint128_t)active_post.total_csaf.value;

   uint128_t  total_csaf = 0;
   uint128_t  last_total_csaf = 0;
   share_type total_effective_csaf = 0;
   uint128_t  turn_point_first = amount * params.approval_casf_first_rate / GRAPHENE_100_PERCENT;
   uint128_t  turn_point_second = amount * params.approval_casf_second_rate / GRAPHENE_100_PERCENT;

   auto get_part_effective_csaf = [=](uint128_t begin, uint128_t end) {
      uint128_t average_point = (begin + end) / 2;
      auto slope = static_cast<int64_t> ((turn_point_second - average_point) * (GRAPHENE_100_PERCENT - params.approval_casf_min_weight) /
         (turn_point_second - turn_point_first) + params.approval_casf_min_weight);
      return static_cast<int64_t>((end - begin) * slope / GRAPHENE_100_PERCENT);
   };
   
   set<std::tuple<score_id_type, share_type, bool>> effective_csaf_container;

   const auto& index = get_index_type<score_index>().indices().get<by_period_sequence>();
   auto itr = index.lower_bound(std::make_tuple(
                                  active_post.platform,  
                                  active_post.poster, 
                                  active_post.post_pid, 
                                  active_post.period_sequence));
   
   while (itr != index.end() && itr->platform == active_post.platform && itr->poster == active_post.poster && 
      itr->post_pid == active_post.post_pid && itr->period_sequence == active_post.period_sequence)
   {        
      total_csaf = total_csaf + itr->csaf.value;
      share_type effective_casf = 0;
      if (total_csaf <= turn_point_first)
      {
         effective_casf = itr->csaf;
      }
      else if (total_csaf <= turn_point_second)
      {
         if (last_total_csaf < turn_point_first)
         {
            effective_casf = static_cast<int64_t>(turn_point_first - last_total_csaf);
            effective_casf += get_part_effective_csaf(turn_point_first, total_csaf);
         }
         else
            effective_casf = get_part_effective_csaf(last_total_csaf, total_csaf);      
      }
      else//total_csaf > turn_point_second
      {
         if (last_total_csaf < turn_point_first)
         {
            effective_casf +=static_cast<int64_t> (turn_point_first - last_total_csaf);
            effective_casf += get_part_effective_csaf(turn_point_first, turn_point_second);            
            effective_casf += static_cast<int64_t>((total_csaf - turn_point_second) * params.approval_casf_min_weight / GRAPHENE_100_PERCENT);
         }
         else if (last_total_csaf < turn_point_second)
         {
            effective_casf += get_part_effective_csaf(last_total_csaf, turn_point_second);       
            effective_casf +=static_cast<int64_t> ((total_csaf - turn_point_second) * params.approval_casf_min_weight / GRAPHENE_100_PERCENT);
         }
         else
         {
            effective_casf = itr->csaf * params.approval_casf_min_weight / GRAPHENE_100_PERCENT;
         }        
      }

      last_total_csaf = last_total_csaf + itr->csaf.value;
      total_effective_csaf = total_effective_csaf + effective_casf;
      
      //bool approve = (score_obj.csaf * score_obj.score * params.casf_modulus / (5 * GRAPHENE_100_PERCENT)) >= 0;
      effective_csaf_container.emplace(std::make_tuple(itr->id, effective_casf, itr->score >= 0));

      ++itr;
   }

   return std::make_tuple(effective_csaf_container, total_effective_csaf);
}

void database::clear_expired_scores()
{
   const auto& global_params = get_global_properties().parameters.get_extension_params();
	const auto& score_expiration_index = get_index_type<score_index>().indices().get<by_create_time>();

	while (!score_expiration_index.empty() && score_expiration_index.begin()->create_time <= head_block_time()-global_params.approval_expiration)
	{
		const score_object& score = *score_expiration_index.begin();
		remove(score);
	}
}

void database::clear_expired_limit_orders()
{
   const auto& limit_order_expiration_index = get_index_type<limit_order_index>().indices().get<by_expiration>();

   while (!limit_order_expiration_index.empty() && limit_order_expiration_index.begin()->expiration <= head_block_time())
   {
      const limit_order_object& limit_order = *limit_order_expiration_index.begin();
      cancel_limit_order(limit_order);
   }
}

void database::update_maintenance_flag( bool new_maintenance_flag )
{
   modify( get_dynamic_global_properties(), [&]( dynamic_global_property_object& dpo )
   {
      auto maintenance_flag = dynamic_global_property_object::maintenance_flag;
      dpo.dynamic_flags =
           (dpo.dynamic_flags & ~maintenance_flag)
         | (new_maintenance_flag ? maintenance_flag : 0);
   } );
   return;
}

void database::clear_expired_csaf_leases()
{
   const uint64_t csaf_window = get_global_properties().parameters.csaf_accumulate_window;
   const auto head_time = head_block_time();
   const auto& idx = get_index_type<csaf_lease_index>().indices().get<by_expiration>();
   auto itr = idx.begin();
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   while( itr != idx.end() && itr->expiration <= head_time )
   {
      modify(get_account_statistics_by_uid(itr->from), [&](_account_statistics_object& s) {
         if (dpo.enabled_hardfork_version < ENABLE_HEAD_FORK_05)
            s.update_coin_seconds_earned(csaf_window, head_time, *this, dpo.enabled_hardfork_version);
         s.core_leased_out -= itr->amount;
      });
      modify(get_account_statistics_by_uid(itr->to), [&](_account_statistics_object& s) {
         if (dpo.enabled_hardfork_version < ENABLE_HEAD_FORK_05)
            s.update_coin_seconds_earned(csaf_window, head_time, *this, dpo.enabled_hardfork_version);
         s.core_leased_in -= itr->amount;
      });
      remove( *itr );
      itr = idx.begin();
   }
}

void database::update_average_witness_pledges()
{
   const auto head_num = head_block_num();
   const auto& idx = get_index_type<witness_index>().indices().get<by_pledge_next_update>();
   auto itr = idx.begin();
   while( itr != idx.end() && itr->average_pledge_next_update_block <= head_num && itr->is_valid )
   {
      update_witness_avg_pledge( *itr );
      itr = idx.begin();
   }
}

void database::update_average_platform_pledges()
{
   const auto head_num = head_block_num();
   const auto& idx = get_index_type<platform_index>().indices().get<by_pledge_next_update>();
   auto itr = idx.begin();
   while (itr != idx.end() && itr->average_pledge_next_update_block <= head_num && itr->is_valid)
   {
      update_platform_avg_pledge(*itr);
      itr = idx.begin();
   }
}

void database::clear_resigned_witness_votes()
{
   const uint32_t max_votes_to_process = GRAPHENE_MAX_RESIGNED_WITNESS_VOTES_PER_BLOCK;
   uint32_t votes_processed = 0;
   const auto& wit_idx = get_index_type<witness_index>().indices().get<by_valid>();
   const auto& vote_idx = get_index_type<witness_vote_index>().indices().get<by_witness_seq>();
   auto wit_itr = wit_idx.begin(); // assume that false < true
   while( wit_itr != wit_idx.end() && wit_itr->is_valid == false )
   {
      auto vote_itr = vote_idx.lower_bound( std::make_tuple( wit_itr->account, wit_itr->sequence ) );
      while( vote_itr != vote_idx.end()
            && vote_itr->witness_uid == wit_itr->account
            && vote_itr->witness_sequence == wit_itr->sequence )
      {
         const voter_object* voter = find_voter( vote_itr->voter_uid, vote_itr->voter_sequence );
         modify( *voter, [&]( voter_object& v )
         {
            v.number_of_witnesses_voted -= 1;
         } );

         auto tmp_itr = vote_itr;
         ++vote_itr;
         remove( *tmp_itr );

         votes_processed += 1;
         if( votes_processed >= max_votes_to_process )
         {
            ilog( "On block ${n}, reached threshold while removing votes for resigned witnesses", ("n",head_block_num()) );
            return;
         }
      }

      update_pledge_mining_bonus_by_witness(*wit_itr);
      //before remove witness, update pledge mining to zero  
      resign_pledge_mining(*wit_itr);
      remove( *wit_itr );

      wit_itr = wit_idx.begin();
   }
}

void database::clear_resigned_committee_member_votes()
{
   const uint32_t max_votes_to_process = GRAPHENE_MAX_RESIGNED_COMMITTEE_VOTES_PER_BLOCK;
   uint32_t votes_processed = 0;
   const auto& com_idx = get_index_type<committee_member_index>().indices().get<by_valid>();
   const auto& vote_idx = get_index_type<committee_member_vote_index>().indices().get<by_committee_member_seq>();
   auto com_itr = com_idx.begin(); // assume that false < true
   while( com_itr != com_idx.end() && com_itr->is_valid == false )
   {
      auto vote_itr = vote_idx.lower_bound( std::make_tuple( com_itr->account, com_itr->sequence ) );
      while( vote_itr != vote_idx.end()
            && vote_itr->committee_member_uid == com_itr->account
            && vote_itr->committee_member_sequence == com_itr->sequence )
      {
         const voter_object* voter = find_voter( vote_itr->voter_uid, vote_itr->voter_sequence );
         modify( *voter, [&]( voter_object& v )
         {
            v.number_of_committee_members_voted -= 1;
         } );

         auto tmp_itr = vote_itr;
         ++vote_itr;
         remove( *tmp_itr );

         votes_processed += 1;
         if( votes_processed >= max_votes_to_process )
         {
            ilog( "On block ${n}, reached threshold while removing votes for resigned committee members", ("n",head_block_num()) );
            return;
         }
      }

      remove( *com_itr );

      com_itr = com_idx.begin();
   }
}

void database::update_voter_effective_votes()
{
   const auto head_num = head_block_num();
   const auto& idx = get_index_type<voter_index>().indices().get<by_votes_next_update>();
   auto itr = idx.begin();
   while( itr != idx.end() && itr->effective_votes_next_update_block <= head_num )
   {
      update_voter_effective_votes( *itr );
      itr = idx.begin();
   }
}

void database::invalidate_expired_governance_voters()
{
   const auto expire_blocks = get_global_properties().parameters.governance_voting_expiration_blocks;
   const auto head_num = head_block_num();
   if( head_num < expire_blocks )
      return;
   const auto max_last_vote_block = head_num - expire_blocks;

   uint32_t voters_processed = 0;
   const auto& idx = get_index_type<voter_index>().indices().get<by_valid>();
   auto itr = idx.lower_bound( std::make_tuple( true, GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID ) );
   while( itr != idx.end() && itr->is_valid && itr->proxy_uid == GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID
         && itr->effective_last_vote_block <= max_last_vote_block )
   {
      ++voters_processed;
      const voter_object& voter = *itr;
      ++itr;
      // this voter become invalid.
      invalidate_voter( voter );
   }
   if( voters_processed > 0 )
      ilog( "Invalidated ${n} expired voters", ("n",voters_processed) );
}

void database::process_invalid_governance_voters()
{
   const uint32_t max_voters_to_process = GRAPHENE_MAX_EXPIRED_VOTERS_TO_PROCESS_PER_BLOCK;
   uint32_t voters_processed = 0;
   const auto& idx = get_index_type<voter_index>().indices().get<by_valid>();
   auto itr = idx.begin(); // assume that false < true
   while( voters_processed < max_voters_to_process && itr != idx.end() && itr->is_valid == false )
   {
      // if there is an invalid voter, process the voters who set it as proxy
      voters_processed += process_invalid_proxied_voters( *itr, max_voters_to_process - voters_processed );
      itr = idx.begin(); // this result should be different if still voters_processed < max_voters_to_process
   }
   if( voters_processed >= max_voters_to_process )
      ilog( "On block ${n}, reached threshold while processing invalid voters or proxies", ("n",head_block_num()) );
}

void database::update_committee()
{
   const global_property_object& gpo = get_global_properties();
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();

   if( head_block_num() >= dpo.next_committee_update_block )
   {
      // expire all committee proposals
      const auto& idx = get_index_type<committee_proposal_index>().indices();
      for( auto itr = idx.begin(); itr != idx.end(); itr = idx.begin() )
      {
         ilog( "expiring committee proposal #${n}: ${p}", ("n",itr->proposal_number)("p", *itr) );
         remove( *itr );
      }

      // prepare to update active_committee_members
      flat_set<account_uid_type> new_committee;
      new_committee.reserve( gpo.parameters.committee_size );

      // by vote top committee members
      const auto& top_idx = get_index_type<committee_member_index>().indices().get<by_votes>();
      auto top_itr = top_idx.lower_bound( true );
      while( top_itr != top_idx.end() && new_committee.size() < gpo.parameters.committee_size )
      {
         new_committee.insert( top_itr->account );
         ++top_itr;
      }

      // update active_committee_members
      modify( gpo, [&]( global_property_object& gp )
      {
         gp.active_committee_members.swap( new_committee );
      });

      // update dynamic global property object
      modify( dpo, [&]( dynamic_global_property_object& dp )
      {
         dp.next_committee_update_block += gpo.parameters.committee_update_interval;
      });

      ilog( "committee updated on block ${n}, next scheduled update block is ${b}",
            ("n",head_block_num())("b",dpo.next_committee_update_block) );
   }
}

void database::adjust_budgets()
{
   const global_property_object& gpo = get_global_properties();
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   if( head_block_num() >= dpo.next_budget_adjust_block )
   {
      const auto& gparams = gpo.parameters;
      share_type core_reserved = get_core_asset().reserved(*this);
      // Normally shouldn't overflow
      uint32_t blocks_per_year = 86400 * 365 / gparams.block_interval
                               - 86400 * 365 * gparams.maintenance_skip_slots / gparams.maintenance_interval;
      uint64_t new_budget = static_cast<uint64_t>( fc::uint128_t( core_reserved.value ) * gparams.budget_adjust_target
                              / blocks_per_year / GRAPHENE_100_PERCENT );
      modify( dpo, [&]( dynamic_global_property_object& _dpo )
      {
         _dpo.total_budget_per_block = new_budget;
         _dpo.next_budget_adjust_block += gpo.parameters.budget_adjust_interval;
      } );

      ilog( "budgets adjusted on block ${n}, next scheduled adjust block is ${b}",
            ("n",head_block_num())("b",dpo.next_budget_adjust_block) );
   }
}

void database::clear_unapproved_committee_proposals()
{
   const auto head_num = head_block_num();
   const auto& idx = get_index_type<committee_proposal_index>().indices().get<by_approved_closing_block>();
   auto itr = idx.begin(); // assume false < true
   while( itr != idx.end() && itr->is_approved == false && itr->voting_closing_block_num <= head_num )
   {
      ilog( "removing voting closed but still unapproved committee proposal #${n}: ${p}",
            ("n",itr->proposal_number)("p",*itr) );
      remove( *itr );
      itr = idx.begin();
   }
}

void database::execute_committee_proposals()
{
   const auto head_num = head_block_num();
   const auto& idx = get_index_type<committee_proposal_index>().indices().get<by_approved_execution_block>();
   auto itr = idx.lower_bound( true );
   while( itr != idx.end() && itr->is_approved == true && itr->execution_block_num <= head_num )
   {
      ilog( "executing committee proposal #${n}: ${p}", ("n",itr->proposal_number)("p",*itr) );
      auto old_itr = itr;
      ++itr;
      execute_committee_proposal( *old_itr, true ); // the 2nd param is true, which means if it fail, won't throw fc::exception
   }
}

void database::execute_committee_proposal( const committee_proposal_object& proposal, bool silent_fail )
{
   try
   {
      FC_ASSERT( proposal.is_approved, "proposal should have been approved by the committee" );
      FC_ASSERT( head_block_num() >= proposal.execution_block_num, "has not yet reached execution block number" );

      // check registrar takeovers, and prepare for objects to be updated
      flat_map<account_uid_type, const account_object*> accounts;
      flat_map<account_uid_type, bool> account_is_registrar;
      flat_map<account_uid_type, account_uid_type> takeover_map;
      flat_map<account_uid_type, committee_update_account_priviledge_item_type::account_priviledge_update_options > account_items;
      const committee_update_fee_schedule_item_type* fee_item = nullptr;
      const committee_update_global_parameter_item_type* param_item = nullptr;
      const committee_update_global_extension_parameter_item_type* extension_parm_item = nullptr;
      for( const auto& item : proposal.items )
      {
         // account update item
         if( item.which() == committee_proposal_item_type::tag< committee_update_account_priviledge_item_type >::value )
         {
            const auto& account_item = item.get< committee_update_account_priviledge_item_type >();
            const auto& pv = account_item.new_priviledges.value;

            bool first_takeover = false;
            account_uid_type first_takeover_registrar = 0;
            if( account_items.find( account_item.account ) == account_items.end() ) // first time process this account
            {
               account_items[ account_item.account ] = account_item.new_priviledges.value;
               if( pv.is_registrar.valid() && *pv.is_registrar == false )
               {
                  first_takeover = true;
                  FC_ASSERT( pv.takeover_registrar.valid(), "Should have takeover registrar account" );
                  first_takeover_registrar = *pv.takeover_registrar;
               }
            }
            else // this account has been already processed at least once
            {
               auto& mv = account_items[ account_item.account ];
               if( pv.can_vote.valid() )
                  mv.can_vote = pv.can_vote;
               if( pv.is_admin.valid() )
                  mv.is_admin = pv.is_admin;
               if( pv.is_registrar.valid() )
               {
                  if( *pv.is_registrar == false && !mv.is_registrar.valid() ) // if it's the first time to be taken-over
                  {
                     first_takeover = true;
                     FC_ASSERT( pv.takeover_registrar.valid(), "Should have takeover registrar account" );
                     first_takeover_registrar = *pv.takeover_registrar;
                  }
                  mv.is_registrar = pv.is_registrar;
               }
            }

            // cache new takeovers
            if( first_takeover )
            {
               const auto& idx = get_index_type<registrar_takeover_index>().indices().get<by_takeover>();
               auto itr = idx.lower_bound( account_item.account );
               while( itr != idx.end() && itr->takeover_registrar == account_item.account )
               {
                  takeover_map[ itr->original_registrar ] = first_takeover_registrar;
                  ++itr;
               }
            }

            if( accounts.find( account_item.account ) == accounts.end() )
            {
               const account_object* account = &get_account_by_uid( account_item.account );
               accounts[ account_item.account ] = account;
               account_is_registrar[ account_item.account ] = account->is_registrar;
            }

            if( pv.is_registrar.valid() )
            {
               account_is_registrar[ account_item.account ] = *pv.is_registrar;
               if( *pv.is_registrar == true )
                  takeover_map.erase( account_item.account );
            }

            if( pv.takeover_registrar.valid() )
            {
               FC_ASSERT( account_is_registrar[ account_item.account ] == false,
                          "Should not take over an active registrar" );

               if( accounts.find( *pv.takeover_registrar ) != accounts.end() )
                  FC_ASSERT( account_is_registrar[ *pv.takeover_registrar ] == true,
                             "Takeover account should be a registrar already" );
               else
               {
                  const account_object* takeover_account = &get_account_by_uid( *pv.takeover_registrar );
                  FC_ASSERT( takeover_account->is_registrar == true,
                             "Takeover account should be a registrar already" );
                  accounts[ takeover_account->uid ] = takeover_account;
                  account_is_registrar[ takeover_account->uid ] = takeover_account->is_registrar;
               }

               // update cache
               for( auto& t : takeover_map )
               {
                  if( t.second == account_item.account )
                     t.second = *pv.takeover_registrar;
               }
               takeover_map[ account_item.account ] = *pv.takeover_registrar;
            }
         }
         // fee update item
         else if( item.which() == committee_proposal_item_type::tag< committee_update_fee_schedule_item_type >::value )
         {
            fee_item = &item.get< committee_update_fee_schedule_item_type >();
         }
         // parameter update item
         else if (item.which() == committee_proposal_item_type::tag< committee_update_global_parameter_item_type >::value)
         {
            param_item = &item.get< committee_update_global_parameter_item_type >();
         }
         else if (item.which() == committee_proposal_item_type::tag< committee_update_global_extension_parameter_item_type >::value)
         {
            extension_parm_item = &item.get< committee_update_global_extension_parameter_item_type >();
         }
         else if (item.which() == committee_proposal_item_type::tag< committee_withdraw_platform_pledge_item_type >::value)
         {
            const auto& platform_punish_item = item.get< committee_withdraw_platform_pledge_item_type >();
            const auto& account_stats = get_account_statistics_by_uid(platform_punish_item.platform_account);
            if (account_stats.pledge_balance_ids.count(pledge_balance_type::Platform))// platform pledge object is nonexistent, invalid proposal
            {
               const auto& pledge_balance_obj = get(account_stats.pledge_balance_ids.at(pledge_balance_type::Platform));
               auto total_unrelease_pledge = pledge_balance_obj.total_unrelease_pledge();

               share_type actual_withdraw_amount = std::min(total_unrelease_pledge, platform_punish_item.withdraw_amount);

               if (total_unrelease_pledge >= actual_withdraw_amount) // platform pledge already release, invalid proposal
               {
                  // withdraw platform account pledge         
                  modify(pledge_balance_obj, [&](pledge_balance_object& _pbo) {
                     share_type from_releasing = std::min(pledge_balance_obj.total_releasing_pledge, actual_withdraw_amount);
                     share_type from_pledge = actual_withdraw_amount - from_releasing;
                     if (from_releasing > 0)
                        _pbo.reduce_releasing(from_releasing);
                     if (from_pledge > 0)
                        _pbo.pledge -= from_pledge;

                     const auto& global_params = get_global_properties().parameters;
                     if (_pbo.pledge < global_params.platform_min_pledge)
                     {
                        if (_pbo.pledge > 0)
                        {
                           auto release_num = head_block_num() + global_params.platform_pledge_release_delay;
                           _pbo.update_pledge(asset(0), release_num, *this);
                        }
                        // platform pledge is below platform min pledge, need delete platform object
                        const platform_object* maybe_found = find_platform_by_owner(platform_punish_item.platform_account);
                        if (maybe_found != nullptr)
                           modify(*maybe_found, [&](platform_object& pfo) {
                              pfo.is_valid = false;
                              pfo.average_pledge_next_update_block = -1;
                        });

                        const auto& account_obj = get_account_by_uid(platform_punish_item.platform_account);
                        modify(account_obj, [&](account_object& acc){
                           acc.is_full_member = false;
                        });
                     }
                     else if (from_pledge > 0) {
                        // update platform data
                        const platform_object& pla_obj = get_platform_by_owner(platform_punish_item.platform_account);
                        update_platform_avg_pledge(pla_obj);
                        modify(pla_obj, [&](platform_object& pfo) {
                           pfo.pledge = _pbo.pledge.value;
                           pfo.last_update_time = head_block_time();
                           pfo.pledge_last_update = head_block_time();
                        });
                        
                     }
                  });
                  // withdraw amount awarded to receiver
                  adjust_balance(platform_punish_item.receiver, asset(actual_withdraw_amount));
               }
            }
         }
      }

      // apply changes : new takeover registrars
      for( const auto& takeover_item : takeover_map )
      {
         const registrar_takeover_object* t = find_registrar_takeover_object( takeover_item.first );
         if( t == nullptr )
         {
            create<registrar_takeover_object>( [&](registrar_takeover_object& o){
               o.original_registrar = takeover_item.first;
               o.takeover_registrar = takeover_item.second;
            });
         }
         else
         {
            modify( *t, [&](registrar_takeover_object& o){
               o.takeover_registrar = takeover_item.second;
            });
         }
      }
      // apply changes : account updates
      for( const auto& account_item : account_items )
      {
         const auto& pv = account_item.second;
         if( pv.is_admin.valid() || pv.is_registrar.valid() )
         {
            const account_object* acc = accounts[ account_item.first ];
            modify( *acc, [&](account_object& a){
               if( pv.is_admin.valid() )
                  a.is_admin = *pv.is_admin;
               if( pv.is_registrar.valid() )
                  a.is_registrar = *pv.is_registrar;
               a.last_update_time = head_block_time();
            });
            if( pv.is_registrar.valid() && *pv.is_registrar == true )
            {
               const registrar_takeover_object* t = find_registrar_takeover_object( account_item.first );
               if( t != nullptr )
                  remove( *t );
            }
         }
         if( pv.can_vote.valid() )
         {
            const _account_statistics_object& st = get_account_statistics_by_uid(account_item.first);
            if( *pv.can_vote == false && st.is_voter == true )
               invalidate_voter( *find_voter( st.owner, st.last_voter_sequence )  );
            modify(st, [&](_account_statistics_object& a){
               a.can_vote = *pv.can_vote;
            });
         }
      }
      // apply changes : fee schedule update
      if( fee_item != nullptr )
      {
         modify( get_global_properties(), [&]( global_property_object& o )
         {
            auto& cp = o.parameters.get_mutable_fees().parameters;
            for( const auto& f : (*fee_item)->parameters )
            {
               fee_parameters params; params.set_which(f.which());
               auto itr = cp.find(params);
               if( itr != cp.end() )
                  *itr = f;
               else
                  cp.insert( f );
            }
         });
      }
      // apply changes : global params update
      if (param_item != nullptr)
      {
         const auto& pv = param_item->value;
         modify(get_global_properties(), [&](global_property_object& _gpo)
         {
            auto& o = _gpo.parameters;
            if (pv.maximum_transaction_size.valid())
               o.maximum_transaction_size = *pv.maximum_transaction_size;
            if (pv.maximum_block_size.valid())
               o.maximum_block_size = *pv.maximum_block_size;
            if (pv.maximum_time_until_expiration.valid())
               o.maximum_time_until_expiration = *pv.maximum_time_until_expiration;
            if (pv.maximum_authority_membership.valid())
               o.maximum_authority_membership = *pv.maximum_authority_membership;
            if (pv.max_authority_depth.valid())
               o.max_authority_depth = *pv.max_authority_depth;
            if (pv.csaf_rate.valid())
               o.csaf_rate = *pv.csaf_rate;
            if (pv.max_csaf_per_account.valid())
               o.max_csaf_per_account = *pv.max_csaf_per_account;
            if (pv.csaf_accumulate_window.valid())
               o.csaf_accumulate_window = *pv.csaf_accumulate_window;
            if (pv.min_witness_pledge.valid())
               o.min_witness_pledge = *pv.min_witness_pledge;
            if (pv.max_witness_pledge_seconds.valid())
               o.max_witness_pledge_seconds = *pv.max_witness_pledge_seconds;
            if (pv.witness_avg_pledge_update_interval.valid())
               o.witness_avg_pledge_update_interval = *pv.witness_avg_pledge_update_interval;
            if (pv.witness_pledge_release_delay.valid())
               o.witness_pledge_release_delay = *pv.witness_pledge_release_delay;
            if (pv.min_governance_voting_balance.valid())
               o.min_governance_voting_balance = *pv.min_governance_voting_balance;
            if (pv.governance_voting_expiration_blocks.valid())
               o.governance_voting_expiration_blocks = *pv.governance_voting_expiration_blocks;
            if (pv.governance_votes_update_interval.valid())
               o.governance_votes_update_interval = *pv.governance_votes_update_interval;
            if (pv.max_governance_votes_seconds.valid())
               o.max_governance_votes_seconds = *pv.max_governance_votes_seconds;
            if (pv.max_witnesses_voted_per_account.valid())
               o.max_witnesses_voted_per_account = *pv.max_witnesses_voted_per_account;
            if (pv.max_witness_inactive_blocks.valid())
               o.max_witness_inactive_blocks = *pv.max_witness_inactive_blocks;
            if (pv.by_vote_top_witness_pay_per_block.valid())
               o.by_vote_top_witness_pay_per_block = *pv.by_vote_top_witness_pay_per_block;
            if (pv.by_vote_rest_witness_pay_per_block.valid())
               o.by_vote_rest_witness_pay_per_block = *pv.by_vote_rest_witness_pay_per_block;
            if (pv.by_pledge_witness_pay_per_block.valid())
               o.by_pledge_witness_pay_per_block = *pv.by_pledge_witness_pay_per_block;
            if (pv.by_vote_top_witness_count.valid())
               o.by_vote_top_witness_count = *pv.by_vote_top_witness_count;
            if (pv.by_vote_rest_witness_count.valid())
               o.by_vote_rest_witness_count = *pv.by_vote_rest_witness_count;
            if (pv.by_pledge_witness_count.valid())
               o.by_pledge_witness_count = *pv.by_pledge_witness_count;
            if (pv.budget_adjust_interval.valid())
               o.budget_adjust_interval = *pv.budget_adjust_interval;
            if (pv.budget_adjust_target.valid())
               o.budget_adjust_target = *pv.budget_adjust_target;
            if (pv.min_committee_member_pledge.valid())
               o.min_committee_member_pledge = *pv.min_committee_member_pledge;
            if (pv.committee_member_pledge_release_delay.valid())
               o.committee_member_pledge_release_delay = *pv.committee_member_pledge_release_delay;
            if (pv.witness_report_prosecution_period.valid())
               o.witness_report_prosecution_period = *pv.witness_report_prosecution_period;
            if (pv.witness_report_allow_pre_last_block.valid())
               o.witness_report_allow_pre_last_block = *pv.witness_report_allow_pre_last_block;
            if (pv.witness_report_pledge_deduction_amount.valid())
               o.witness_report_pledge_deduction_amount = *pv.witness_report_pledge_deduction_amount;

            if (pv.platform_min_pledge.valid())
               o.platform_min_pledge = *pv.platform_min_pledge;
            if (pv.platform_pledge_release_delay.valid())
               o.platform_pledge_release_delay = *pv.platform_pledge_release_delay;
            if (pv.platform_max_vote_per_account.valid())
               o.platform_max_vote_per_account = *pv.platform_max_vote_per_account;
            if (pv.platform_max_pledge_seconds.valid())
               o.platform_max_pledge_seconds = *pv.platform_max_pledge_seconds;
            if (pv.platform_avg_pledge_update_interval.valid())
               o.platform_avg_pledge_update_interval = *pv.platform_avg_pledge_update_interval;
         });
      }
      if (extension_parm_item != nullptr)
      {
         const auto& pv = extension_parm_item->value;
         modify(get_global_properties(), [&](global_property_object& _gpo)
         {
            auto& v = _gpo.parameters.extension_parameters;
            if (pv.content_award_interval.valid())
               v.content_award_interval = *pv.content_award_interval;
            if (pv.platform_award_interval.valid())
               v.platform_award_interval = *pv.platform_award_interval;
            if (pv.max_csaf_per_approval.valid())
               v.max_csaf_per_approval = *pv.max_csaf_per_approval;
            if (pv.approval_expiration.valid())
               v.approval_expiration = *pv.approval_expiration;
            if (pv.min_effective_csaf.valid())
               v.min_effective_csaf = *pv.min_effective_csaf;
            if (pv.total_content_award_amount.valid())
               v.total_content_award_amount = *pv.total_content_award_amount;
            if (pv.total_platform_content_award_amount.valid())
               v.total_platform_content_award_amount = *pv.total_platform_content_award_amount;
            if (pv.total_platform_voted_award_amount.valid())
               v.total_platform_voted_award_amount = *pv.total_platform_voted_award_amount;
            if (pv.platform_award_min_votes.valid())
               v.platform_award_min_votes = *pv.platform_award_min_votes;
            if (pv.platform_award_requested_rank.valid())
               v.platform_award_requested_rank = *pv.platform_award_requested_rank;

            if (pv.platform_award_basic_rate.valid())
               v.platform_award_basic_rate = *pv.platform_award_basic_rate;
            if (pv.casf_modulus.valid())
               v.casf_modulus = *pv.casf_modulus;
            if (pv.post_award_expiration.valid())
               v.post_award_expiration = *pv.post_award_expiration;
            if (pv.approval_casf_min_weight.valid())
               v.approval_casf_min_weight = *pv.approval_casf_min_weight;
            if (pv.approval_casf_first_rate.valid())
               v.approval_casf_first_rate = *pv.approval_casf_first_rate;
            if (pv.approval_casf_second_rate.valid())
               v.approval_casf_second_rate = *pv.approval_casf_second_rate;
            if (pv.receiptor_award_modulus.valid())
               v.receiptor_award_modulus = *pv.receiptor_award_modulus;
            if (pv.disapprove_award_modulus.valid())
               v.disapprove_award_modulus = *pv.disapprove_award_modulus;

            if (pv.advertising_confirmed_fee_rate.valid())
               v.advertising_confirmed_fee_rate = *pv.advertising_confirmed_fee_rate;
            if (pv.advertising_confirmed_min_fee.valid())
               v.advertising_confirmed_min_fee = *pv.advertising_confirmed_min_fee;
            if (pv.custom_vote_effective_time.valid())
               v.custom_vote_effective_time = *pv.custom_vote_effective_time;

            if (pv.min_witness_block_produce_pledge.valid())
               v.min_witness_block_produce_pledge = *pv.min_witness_block_produce_pledge;

            if (pv.content_award_skip_slots.valid())
               v.content_award_skip_slots = *pv.content_award_skip_slots;
            if (pv.unlocked_balance_release_delay.valid())
               v.unlocked_balance_release_delay = *pv.unlocked_balance_release_delay;
            if (pv.min_mining_pledge.valid())
               v.min_mining_pledge = *pv.min_mining_pledge;
            if (pv.mining_pledge_release_delay.valid())
               v.mining_pledge_release_delay = *pv.mining_pledge_release_delay;
            if (pv.max_pledge_mining_bonus_rate.valid())
               v.max_pledge_mining_bonus_rate = *pv.max_pledge_mining_bonus_rate;
            if (pv.registrar_referrer_rate_from_score.valid())
               v.registrar_referrer_rate_from_score = *pv.registrar_referrer_rate_from_score;
            if (pv.max_pledge_releasing_size.valid())
               v.max_pledge_releasing_size = *pv.max_pledge_releasing_size;
            if (pv.scorer_earnings_rate.valid())
               v.scorer_earnings_rate = *pv.scorer_earnings_rate;
            if (pv.platform_content_award_min_votes.valid())
               v.platform_content_award_min_votes = *pv.platform_content_award_min_votes;
            if (pv.csaf_limit_lock_balance_modulus.valid())
               v.csaf_limit_lock_balance_modulus = *pv.csaf_limit_lock_balance_modulus;
         });
      }

      // remove the executed proposal
      remove(proposal);

   } catch ( const fc::exception& e ) {
      if( silent_fail )
      {
         if( proposal.execution_block_num >= proposal.expiration_block_num
               || proposal.expiration_block_num <= head_block_num() )
         {
            wlog( "exception thrown while executing committee proposal ${p} :\n${e}\nexpired, removing.",
                  ("p",proposal)("e",e.to_detail_string() ) );
            try {
               remove( proposal );
            } catch( ... ) {}
         }
         else
         {
            wlog( "exception thrown while executing committee proposal ${p} :\n${e}\nwill try again on expiration block #${n}.",
                  ("p",proposal)("e",e.to_detail_string() )("n",proposal.expiration_block_num) );
            try {
               modify( proposal, [&]( committee_proposal_object& cpo ){
                  cpo.execution_block_num = cpo.expiration_block_num;
               });
            } catch( ... ) {}
         }
      }
      else
      {
         wlog( "exception thrown while executing committee proposal ${p} :\n${e}", ("p",proposal)("e",e.to_detail_string() ) );
         throw e;
      }
   }
}

void database::check_invariants()
{
   const auto head_num = head_block_num();
   const global_property_object& gpo = get_global_properties();
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   const witness_schedule_object& wso = witness_schedule_id_type()(*this);

   FC_ASSERT( dpo.budget_pool >= 0 );
   FC_ASSERT( dpo.next_budget_adjust_block > head_num );
   FC_ASSERT( dpo.next_committee_update_block > head_num );
   FC_ASSERT( wso.next_schedule_block_num > head_num );
   //if( head_block_num() >= 1285 ) { idump( (dpo) ); }

   map<asset_aid_type, share_type> total_balances;
   const auto& balance_index = get_index_type<account_balance_index>().indices();
   for (const account_balance_object& b : balance_index)
      total_balances[b.asset_type] += b.balance;

   share_type total_core_balance = 0;
   share_type total_core_non_bal = dpo.budget_pool;
   share_type total_core_leased_in = 0;
   share_type total_core_leased_out = 0;
   share_type total_core_witness_pledge = 0;
   share_type total_core_committee_member_pledge = 0;
   share_type total_core_platform_pledge = 0;

   uint64_t total_voting_accounts = 0;
   share_type total_voting_core_balance = 0;

   const auto& acc_stats_idx = get_index_type<account_statistics_index>().indices();
   for( const auto& s: acc_stats_idx )
   {
      //if( head_block_num() >= 1285 ) { idump( (s) ); }
      FC_ASSERT( s.core_balance == get_balance( s.owner, GRAPHENE_CORE_ASSET_AID ).amount );
      FC_ASSERT( s.core_balance >= 0 );
      FC_ASSERT( s.prepaid >= 0 );
      FC_ASSERT( s.csaf >= 0 );
      FC_ASSERT( s.core_leased_in >= 0 );
      FC_ASSERT( s.core_leased_out >= 0 );

      for (const auto& id : s.pledge_balance_ids)
      {
         auto pledge_balance_obj = get(id.second);
         for (auto iter_pledge : pledge_balance_obj.releasing_pledges){
            FC_ASSERT(iter_pledge.first > head_num);
         }
      }

      for (const auto & p : s.uncollected_market_fees)
         total_balances[p.first] += p.second;

      auto iter_fee = s.uncollected_market_fees.find(GRAPHENE_CORE_ASSET_AID);
      share_type uncollect_market_fee = iter_fee != s.uncollected_market_fees.end() ? iter_fee->second : 0;

      total_core_balance += s.core_balance;
      total_core_non_bal += (s.prepaid + s.uncollected_witness_pay + s.uncollected_pledge_bonus + s.uncollected_score_bonus + uncollect_market_fee);
      total_core_leased_in += s.core_leased_in;
      total_core_leased_out += s.core_leased_out;
      if (s.pledge_balance_ids.count(pledge_balance_type::Witness))
         total_core_witness_pledge += get(s.pledge_balance_ids.at(pledge_balance_type::Witness)).pledge;
      if (s.pledge_balance_ids.count(pledge_balance_type::Commitment))
         total_core_committee_member_pledge += get(s.pledge_balance_ids.at(pledge_balance_type::Commitment)).pledge;
      if (s.pledge_balance_ids.count(pledge_balance_type::Platform))
         total_core_platform_pledge += get(s.pledge_balance_ids.at(pledge_balance_type::Platform)).pledge;
      FC_ASSERT(s.core_balance >= s.core_leased_out + s.total_mining_pledge + s.get_all_pledge_balance(GRAPHENE_CORE_ASSET_AID, *this));

      if( s.is_voter )
      {
         ++total_voting_accounts;
         total_voting_core_balance += s.get_votes_from_core_balance();
      }
   }

   for (const limit_order_object& o : get_index_type<limit_order_index>().indices())
   {
      asset for_sale = o.amount_for_sale();
      total_balances[for_sale.asset_id] += for_sale.amount;
   }

   for (const asset_object& asset_obj : get_index_type<asset_index>().indices())
   {
      total_balances[asset_obj.asset_id] += asset_obj.dynamic_data(*this).accumulated_fees.value;
   }

   for (const witness_object& witness_obj : get_index_type<witness_index>().indices())
   {
      total_core_non_bal += (witness_obj.need_distribute_bonus - witness_obj.already_distribute_bonus);
   }

   FC_ASSERT( total_core_leased_in == total_core_leased_out );

   share_type total_advertising_released = 0;
   const auto& adt_idx = get_index_type<advertising_order_index>().indices().get<by_advertising_order_state>();
   auto advertising_iter = adt_idx.lower_bound(advertising_undetermined);
   while (advertising_iter != adt_idx.end() && advertising_iter->status == advertising_undetermined)
   {
       total_advertising_released += advertising_iter->released_balance;
       ++advertising_iter;
   }
   total_balances[GRAPHENE_CORE_ASSET_AID] += total_advertising_released + total_core_non_bal;

   for (const asset_object& asset_obj : get_index_type<asset_index>().indices())
   {
      FC_ASSERT(total_balances[asset_obj.asset_id].value == asset_obj.dynamic_data(*this).current_supply.value);
   }

   share_type total_core_leased = 0;
   const auto& csaf_lease_idx = get_index_type<csaf_lease_index>().indices();
   for( const auto& s: csaf_lease_idx )
   {
      FC_ASSERT( s.amount > 0 );
      total_core_leased += s.amount;
   }
   FC_ASSERT( total_core_leased_out == total_core_leased );

   share_type total_core_balance_indexed = 0;
   const auto& acc_bal_idx = get_index_type<account_balance_index>().indices();
   for( const auto& s: acc_bal_idx )
   {
      FC_ASSERT( s.balance >= 0 );
      if( s.asset_type == GRAPHENE_CORE_ASSET_AID)
         total_core_balance_indexed += s.balance;
   }
   FC_ASSERT( total_core_balance == total_core_balance_indexed );

   uint64_t total_voters = 0;
   uint64_t total_witnesses_voted = 0;
   uint64_t total_committee_members_voted = 0;
   uint64_t total_platform_voted = 0;
   uint64_t total_voter_votes = 0;
   fc::uint128_t total_voter_witness_votes = 0;
   fc::uint128_t total_voter_committee_member_votes = 0;
   fc::uint128_t total_voter_platform_votes = 0;
   vector<share_type> total_got_proxied_votes;
   vector<share_type> total_proxied_votes;
   total_got_proxied_votes.resize( gpo.parameters.max_governance_voting_proxy_level );
   total_proxied_votes.resize( gpo.parameters.max_governance_voting_proxy_level );
   const auto& voter_idx = get_index_type<voter_index>().indices();
   for( const auto& s: voter_idx )
   {
      if( s.is_valid )
      {
         FC_ASSERT( s.effective_votes_next_update_block > head_num );
         const auto& stats = get_account_statistics_by_uid( s.uid );
         FC_ASSERT( stats.last_voter_sequence == s.sequence );
         FC_ASSERT(stats.get_votes_from_core_balance() == s.votes);
         ++total_voters;
         total_voter_votes += s.votes;
         total_witnesses_voted += s.number_of_witnesses_voted;
         total_committee_members_voted += s.number_of_committee_members_voted;
         total_platform_voted += s.number_of_platform_voted;
         if( s.proxy_uid == GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID )
         {
            total_voter_witness_votes += fc::uint128_t( s.total_votes() ) * s.number_of_witnesses_voted;
            total_voter_committee_member_votes += fc::uint128_t( s.total_votes() ) * s.number_of_committee_members_voted;
            total_voter_platform_votes += fc::uint128_t( s.total_votes() ) * s.number_of_platform_voted;
         }
         else
         {
            FC_ASSERT( s.number_of_witnesses_voted == 0 );
            FC_ASSERT( s.number_of_committee_members_voted == 0 );
            FC_ASSERT( s.number_of_platform_voted == 0 );
            total_proxied_votes[0] += s.effective_votes;
            for( size_t i = 1; i < gpo.parameters.max_governance_voting_proxy_level; ++i )
               total_proxied_votes[i] += s.proxied_votes[i-1];
         }
         const auto& account = get_account_by_uid(s.uid);
         if (account.referrer_by_platform){
             const platform_object* plat = find_platform_by_sequence(account.reg_info.referrer, account.referrer_by_platform);
             if (plat)
                 total_voter_platform_votes += s.effective_votes;
         }   
         for( size_t i = 0; i < gpo.parameters.max_governance_voting_proxy_level; ++i )
            total_got_proxied_votes[i] += s.proxied_votes[i];
      }
   }
   FC_ASSERT( total_voting_accounts == total_voters );
   FC_ASSERT( total_voting_core_balance == total_voter_votes );
   for( size_t i = 0; i < gpo.parameters.max_governance_voting_proxy_level; ++i )
   {
      FC_ASSERT( total_proxied_votes[i] == total_got_proxied_votes[i] );
   }

   share_type total_witness_pledges;
   fc::uint128_t total_witness_received_votes = 0;
   const auto& wit_idx = get_index_type<witness_index>().indices();
   for( const auto& s: wit_idx )
   {
      if( s.is_valid )
      {
         FC_ASSERT( s.average_pledge_next_update_block > head_num );
         FC_ASSERT( s.by_pledge_scheduled_time >= wso.current_by_pledge_time );
         FC_ASSERT( s.by_vote_scheduled_time >= wso.current_by_vote_time );
         const auto& stats = get_account_statistics_by_uid( s.account );
         FC_ASSERT( stats.last_witness_sequence == s.sequence );
         total_witness_pledges += s.pledge;
         total_witness_received_votes += s.total_votes;
      }
   }
   FC_ASSERT( total_witness_pledges == total_core_witness_pledge );
   FC_ASSERT( total_witness_received_votes == total_voter_witness_votes );

   share_type total_committee_member_pledges;
   fc::uint128_t total_committee_member_received_votes = 0;
   const auto& com_idx = get_index_type<committee_member_index>().indices();
   for( const auto& s: com_idx )
   {
      if( s.is_valid )
      {
         const auto& stats = get_account_statistics_by_uid( s.account );
         FC_ASSERT( stats.last_committee_member_sequence == s.sequence );
         total_committee_member_pledges += s.pledge;
         total_committee_member_received_votes += s.total_votes;
      }
   }
   FC_ASSERT( total_committee_member_pledges == total_core_committee_member_pledge );
   FC_ASSERT( total_committee_member_received_votes == total_voter_committee_member_votes );

   /// platform
   share_type total_platform_pledges;
   fc::uint128_t total_platform_received_votes = 0;
   const auto& pla_idx = get_index_type<platform_index>().indices();
   for( const auto& s: pla_idx )
   {
      if( s.is_valid )
      {
         const auto& stats = get_account_statistics_by_uid( s.owner );
         FC_ASSERT( stats.last_platform_sequence == s.sequence );
         total_platform_pledges += s.pledge;
         total_platform_received_votes += s.total_votes;
      }
   }
   FC_ASSERT( total_platform_pledges == total_core_platform_pledge );
   FC_ASSERT( total_platform_received_votes == total_voter_platform_votes, "t1:${t1}  t2:${t2}",("t1",total_platform_received_votes)("t2",total_voter_platform_votes) );


   uint64_t total_witness_vote_objects = 0;
   const auto& wit_vote_idx = get_index_type<witness_vote_index>().indices();
   for( const auto& s: wit_vote_idx )
   {
      const auto wit = find_witness_by_uid( s.witness_uid );
      const auto voter = find_voter( s.voter_uid, s.voter_sequence );
      if( wit != nullptr && voter != nullptr && voter->is_valid && wit->sequence == s.witness_sequence )
      {
         ++total_witness_vote_objects;
      }
   }
   FC_ASSERT( total_witnesses_voted == total_witness_vote_objects );

   uint64_t total_committee_member_vote_objects = 0;
   const auto& com_vote_idx = get_index_type<committee_member_vote_index>().indices();
   for( const auto& s: com_vote_idx )
   {
      const auto com = find_committee_member_by_uid( s.committee_member_uid );
      const auto voter = find_voter( s.voter_uid, s.voter_sequence );
      if( com != nullptr && voter != nullptr && voter->is_valid && com->sequence == s.committee_member_sequence )
      {
         ++total_committee_member_vote_objects;
      }
   }
   FC_ASSERT( total_committee_members_voted == total_committee_member_vote_objects );

   /// platform
   uint64_t total_platform_vote_objects = 0;
   const auto& pla_vote_idx = get_index_type<platform_vote_index>().indices();
   for( const auto& s: pla_vote_idx )
   {
      const auto pla = find_platform_by_owner( s.platform_owner );
      const auto voter = find_voter( s.voter_uid, s.voter_sequence );
      if( pla != nullptr && voter != nullptr && voter->is_valid && pla->sequence == s.platform_sequence )
      {
         ++total_platform_vote_objects;
      }
   }
   FC_ASSERT( total_platform_voted == total_platform_vote_objects );
}

void database::adjust_platform_votes( const platform_object& platform, share_type delta )
{
   if( delta == 0 || !platform.is_valid )
      return;
   modify( platform, [&]( platform_object& pla )
   {
      pla.total_votes += delta.value;
   } );
}

void database::update_pledge_mining_bonus()
{
   const auto& wit_idx = get_index_type<witness_index>().indices().get<by_pledge_mining_bonus>();
   auto wit_itr = wit_idx.lower_bound(head_block_num());
   vector<std::reference_wrapper<const witness_object>> refs;
   while (wit_itr != wit_idx.end()) {
      update_pledge_mining_bonus_by_witness(*wit_itr);
      refs.emplace_back(std::cref(*wit_itr));
      ++wit_itr;
   }

   std::for_each(refs.begin(), refs.end(), [&](const witness_object& witness_obj) {
      modify(witness_obj, [&](witness_object& wit) {
         wit.unhandled_bonus = 0;
         wit.need_distribute_bonus = 0;
         wit.already_distribute_bonus = 0;
         wit.last_update_bonus_block_num = head_block_num();
         wit.bonus_per_pledge.clear();
      });
   });
}

void database::update_pledge_mining_bonus_by_witness(const witness_object& witness_obj)
{
   share_type send_bonus = 0;
   const auto& pmg_idx = get_index_type<pledge_mining_index>().indices().get<by_pledge_witness>();
   auto pmg_itr = pmg_idx.lower_bound(witness_obj.account);
   while (pmg_itr != pmg_idx.end() && pmg_itr->witness == witness_obj.account)
   {
      share_type bonus_per_pledge = witness_obj.accumulate_bonus_per_pledge(pmg_itr->last_bonus_block_num + 1);
      send_bonus += update_pledge_mining_bonus_by_account(*pmg_itr, bonus_per_pledge);
      ++pmg_itr;
   }
   modify(get_account_statistics_by_uid(witness_obj.account), [&](_account_statistics_object& o)
   {
      o.uncollected_witness_pay += (witness_obj.need_distribute_bonus 
         - witness_obj.already_distribute_bonus 
         - send_bonus);
   });
}

share_type database::update_pledge_mining_bonus_by_account(const pledge_mining_object& pledge_mining_obj, share_type bonus_per_pledge)
{
   if (this->get(pledge_mining_obj.pledge_id).pledge == 0)
      return 0;

   share_type total_bonus = static_cast<int64_t> ((uint128_t)bonus_per_pledge.value * get(pledge_mining_obj.pledge_id).pledge.value
      / GRAPHENE_PLEDGE_BONUS_PRECISION);
   if (total_bonus > 0) {
      modify(get_account_statistics_by_uid(pledge_mining_obj.pledge_account), [&](_account_statistics_object& o) {
         o.uncollected_pledge_bonus += total_bonus;
      });
   }
   modify(pledge_mining_obj, [&](pledge_mining_object& o) {
      o.last_bonus_block_num = head_block_num();
   });
    
   return total_bonus;
}

void database::update_platform_avg_pledge( const account_uid_type uid )
{
   update_platform_avg_pledge( get_platform_by_owner( uid ) );
}

void database::update_platform_avg_pledge( const platform_object& pla )
{
   if( !pla.is_valid )
      return;

   const auto& global_params = get_global_properties().parameters;
   const auto window = global_params.platform_max_pledge_seconds;
   const auto now = head_block_time();

   // update avg pledge
   const auto old_avg_pledge = pla.average_pledge;
   if( pla.average_pledge == pla.pledge )
   {
      modify( pla, [&]( platform_object& p )
      {
         p.average_pledge_last_update = now;
         p.average_pledge_next_update_block = -1;
      } );
   }
   else if( pla.average_pledge > pla.pledge || now >= pla.pledge_last_update + window )
   {
      modify( pla, [&]( platform_object& p )
      {
         p.average_pledge = p.pledge;
         p.average_pledge_last_update = now;
         p.average_pledge_next_update_block = -1;
      } );
   }
   else if( now > pla.average_pledge_last_update )
   {
      // need to schedule next update because average_pledge < pledge, and need to update average_pledge
      uint64_t delta_seconds = ( now - pla.average_pledge_last_update ).to_seconds();
      uint64_t new_average_coins;
      const dynamic_global_property_object& dpo = get_dynamic_global_properties();
      if (dpo.enabled_hardfork_version < ENABLE_HEAD_FORK_05)
      {
         uint64_t old_seconds = window - delta_seconds;

         fc::uint128_t old_coin_seconds = fc::uint128_t( pla.average_pledge ) * old_seconds;
         fc::uint128_t new_coin_seconds = fc::uint128_t( pla.pledge ) * delta_seconds;

         new_average_coins = static_cast<uint64_t>( ( old_coin_seconds + new_coin_seconds ) / window );
      }
      else
      {
         uint64_t total_seconds = window - ( pla.average_pledge_last_update - pla.pledge_last_update ).to_seconds();

         new_average_coins = pla.average_pledge + static_cast<uint64_t> ( fc::uint128_t( pla.pledge - pla.average_pledge ) * delta_seconds / total_seconds );
      }

      modify( pla, [&]( platform_object& p )
      {
         p.average_pledge = new_average_coins;
         p.average_pledge_last_update = now;
         p.average_pledge_next_update_block = head_block_num() + global_params.platform_avg_pledge_update_interval;
      } );
   }
   else
   {
      // need to schedule next update because average_pledge < pledge, but no need to update average_pledge
      modify( pla, [&]( platform_object& p )
      {
         p.average_pledge_next_update_block = head_block_num() + global_params.platform_avg_pledge_update_interval;
      } );
   }

   if( old_avg_pledge != pla.average_pledge )
   {
      // TODO: Adjust distribution logic
   }
}

void database::resign_pledge_mining(const witness_object& wit)
{
   const auto& params = get_global_properties().parameters.get_extension_params();
   const auto& idx = get_index_type<pledge_mining_index>().indices().get<by_pledge_witness>();
   auto itr = idx.lower_bound(wit.account);
   while (itr != idx.end() && itr->witness == wit.account)
   {
      const auto& obj=get(itr->pledge_id);
      modify(obj, [&](pledge_balance_object& s) {
         s.update_pledge(asset(0), head_block_num() + params.mining_pledge_release_delay, *this);
      });

      ++itr;
   }
}

void database::clear_resigned_platform_votes()
{
   const uint32_t max_votes_to_process = GRAPHENE_MAX_RESIGNED_PLATFORM_VOTES_PER_BLOCK;
   uint32_t votes_processed = 0;
   const auto& pla_idx = get_index_type<platform_index>().indices().get<by_valid>();
   const auto& vote_idx = get_index_type<platform_vote_index>().indices().get<by_platform_owner_seq>();
   auto pla_itr = pla_idx.begin(); // assume that false < true
   while( pla_itr != pla_idx.end() && pla_itr->is_valid == false )
   {
      auto vote_itr = vote_idx.lower_bound( std::make_tuple( pla_itr->owner, pla_itr->sequence ) );
      while( vote_itr != vote_idx.end()
            && vote_itr->platform_owner == pla_itr->owner
            && vote_itr->platform_sequence == pla_itr->sequence )
      {
         const voter_object* voter = find_voter( vote_itr->voter_uid, vote_itr->voter_sequence );
         modify( *voter, [&]( voter_object& v )
         {
            v.number_of_platform_voted -= 1;
         } );

         auto tmp_itr = vote_itr;
         ++vote_itr;
         remove( *tmp_itr );

         votes_processed += 1;
         if( votes_processed >= max_votes_to_process )
         {
            ilog( "On block ${n}, reached threshold while removing votes for resigned platforms", ("n",head_block_num()) );
            return;
         }
      }

      remove( *pla_itr );

      pla_itr = pla_idx.begin();
   }
}

void database::process_content_platform_awards()
{ 
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   const auto block_time = head_block_time();
   if (block_time >= dpo.next_content_award_time)
   {
      const global_property_object& gpo = get_global_properties();
      const auto& params = gpo.parameters.get_extension_params();

      if ((params.total_content_award_amount == 0 && params.total_platform_content_award_amount == 0) || params.content_award_interval == 0)
      {
         //close platform and post award
         if (dpo.next_content_award_time != time_point_sec(0))
         {
            clear_active_post();
            modify(dpo, [&](dynamic_global_property_object& _dpo)
            {
               _dpo.last_content_award_time = time_point_sec(0);
               _dpo.next_content_award_time = time_point_sec(0);
               _dpo.content_award_enable = false;
            });
         }
         return;
      }

      if (dpo.next_content_award_time == time_point_sec(0))//start platform and post award
      {
         clear_active_post();
         modify(dpo, [&](dynamic_global_property_object& _dpo)
         {
            _dpo.last_content_award_time = block_time;
            _dpo.next_content_award_time = block_time + params.content_award_interval;
            ++_dpo.current_active_post_sequence;
            _dpo.content_award_enable = true;
         });
         return;
      }

      share_type actual_awards = 0;

      bool can_award = false;
      if (dpo.enabled_hardfork_version >= ENABLE_HEAD_FORK_05)
      {
         fc::uint128_t award_two_periods = (fc::uint128_t)(params.total_content_award_amount + params.total_platform_content_award_amount).value*2*
            (dpo.next_content_award_time - dpo.last_content_award_time).to_seconds() / (86400 * 365);
         can_award = dpo.budget_pool >= static_cast<uint64_t>(award_two_periods);
      } else {
         can_award = dpo.budget_pool >= (params.total_content_award_amount + params.total_platform_content_award_amount);
      }    

      if (can_award)
      {
         //notify witness plugin skip block
         modify(dpo, [&](dynamic_global_property_object& _dpo)
         {
            _dpo.content_award_skip_flag = true;
         });

         share_type total_csaf_amount = 0;
         share_type total_effective_csaf_amount = 0;
         map<account_uid_type, share_type> platform_csaf_amount;
         //<active post object, post effective csaf, (csaf * score / 5)*modulus>
         vector<std::tuple<active_post_object*, share_type, share_type>> post_effective_casf;

         const auto& apt_idx = get_index_type<active_post_index>().indices().get<by_period_sequence>();
         auto apt_itr = apt_idx.lower_bound(dpo.current_active_post_sequence);
         while (apt_itr != apt_idx.end() && apt_itr->period_sequence == dpo.current_active_post_sequence)
         {
            if (dpo.enabled_hardfork_version >= ENABLE_HEAD_FORK_05) 
            {
               const platform_object* pla = find_platform_by_owner(apt_itr->platform);
               if (pla == nullptr || !pla->is_valid || pla->total_votes < params.platform_content_award_min_votes) 
               {
                  ++apt_itr;
                  continue;
               }  
            }

            if (apt_itr->total_csaf >= params.min_effective_csaf)
            {
               const auto& idx = get_index_type<score_index>().indices().get<by_period_sequence>();
               auto itr = idx.lower_bound(std::make_tuple(apt_itr->platform, apt_itr->poster, apt_itr->post_pid, apt_itr->period_sequence));

               boost::multiprecision::int128_t approval_amount = 0;
               while (itr != idx.end() && itr->platform == apt_itr->platform && itr->poster == apt_itr->poster &&
                  itr->post_pid == apt_itr->post_pid && itr->period_sequence == apt_itr->period_sequence)
               {
                  approval_amount += (boost::multiprecision::int128_t)itr->csaf.value * itr->score * params.casf_modulus
                     / (5 * GRAPHENE_100_PERCENT);
                  ++itr;
               }
               share_type csaf = apt_itr->total_csaf + approval_amount.convert_to<int64_t>();
               if (csaf > 0)
               {
                  total_effective_csaf_amount += csaf;
                  post_effective_casf.emplace_back(std::make_tuple((active_post_object*)&(*apt_itr), csaf, approval_amount.convert_to<int64_t>()));
               }
            }

            if (platform_csaf_amount.find(apt_itr->platform) != platform_csaf_amount.end())
               platform_csaf_amount.at(apt_itr->platform) += apt_itr->total_csaf;
            else
               platform_csaf_amount.emplace(apt_itr->platform, apt_itr->total_csaf);
            total_csaf_amount += apt_itr->total_csaf;

            ++apt_itr;
         }

         std::map<account_uid_type, share_type> adjust_balance_map;

         if (params.total_content_award_amount > 0 && total_effective_csaf_amount > 0)
         {
            //compute per period award amount 
            uint128_t content_award_amount_per_period = (uint128_t)(params.total_content_award_amount.value) *
               (dpo.next_content_award_time - dpo.last_content_award_time).to_seconds() / (86400 * 365);

            flat_map<account_uid_type, std::pair<share_type, share_type>> platform_receiptor_award;
            std::map<account_uid_type, share_type> registrar_and_referrer_award;
            for (auto itr = post_effective_casf.begin(); itr != post_effective_casf.end(); ++itr)
            {
               share_type post_earned =static_cast<int64_t> (content_award_amount_per_period * std::get<1>(*itr).value /
                  total_effective_csaf_amount.value);
               share_type score_earned = 0;
               share_type receiptor_earned = 0;
               if (dpo.enabled_hardfork_version < ENABLE_HEAD_FORK_05)
                  score_earned = static_cast<int64_t>((uint128_t)post_earned.value * GRAPHENE_DEFAULT_SCORE_RECEIPTS_RATIO / GRAPHENE_100_PERCENT);
               else
                  score_earned = static_cast<int64_t>((uint128_t)post_earned.value * params.scorer_earnings_rate / GRAPHENE_100_PERCENT);
               if (std::get<2>(*itr) >= 0)
                  receiptor_earned = post_earned - score_earned;
               else
                  receiptor_earned = static_cast<int64_t>((uint128_t)((post_earned - score_earned).value)*params.receiptor_award_modulus / GRAPHENE_100_PERCENT);

               const auto& post = get_post_by_platform(std::get<0>(*itr)->platform, std::get<0>(*itr)->poster, std::get<0>(*itr)->post_pid);
               share_type temp = receiptor_earned;
               flat_map<account_uid_type, share_type> receiptor;
               for (const auto& r : post.receiptors)
               {
                  if (r.first == post.platform)
                     continue;
                  share_type to_add = static_cast<int64_t>((uint128_t)receiptor_earned.value * r.second.cur_ratio / GRAPHENE_100_PERCENT);

                  ///adjust_balance(r.first, asset(to_add));
                  adjust_balance_map[r.first] += to_add;
                  receiptor.emplace(r.first, to_add);
                  temp -= to_add;
               }
               adjust_balance_map[post.platform] += temp;
               receiptor.emplace(post.platform, temp);

               share_type award_only_from_platform;
               if (post.poster == post.platform)
                  award_only_from_platform = static_cast<int64_t>((uint128_t)receiptor_earned.value * GRAPHENE_DEFAULT_PLATFORM_RECEIPTS_RATIO /
                  GRAPHENE_100_PERCENT);
               else
                  award_only_from_platform = temp;
               if (platform_receiptor_award.count(post.platform))
               {
                  platform_receiptor_award.at(post.platform).first += temp;
                  platform_receiptor_award.at(post.platform).second += award_only_from_platform;
               }
               else
               {
                  platform_receiptor_award.emplace(post.platform, std::make_pair(temp, award_only_from_platform));
               }

               modify(*(std::get<0>(*itr)), [&](active_post_object& act)
               {
                  act.positive_win = std::get<2>(*itr) >= 0;
                  act.post_award = receiptor_earned;
                  for (const auto& r : receiptor)
                     act.insert_receiptor(r.first, r.second);
               });

               if (post.score_settlement)
                  continue;
               //result <set<score id, effective csaf for the score, is or not approve>, total effective csaf to award>
               auto result = get_effective_csaf(*(std::get<0>(*itr)));
               uint128_t total_award_csaf = (uint128_t)std::get<1>(result).value;
               share_type actual_score_earned = 0;
               for (const auto& e : std::get<0>(result))
               {
                  uint128_t effective_csaf_per_account = (uint128_t)std::get<1>(e).value;
                  share_type to_add = 0;
                  if (std::get<2>(*itr) < 0 && !std::get<2>(e))
                     to_add = static_cast<int64_t>(effective_csaf_per_account * score_earned.value * params.disapprove_award_modulus /
                     (total_award_csaf * GRAPHENE_100_PERCENT));
                  else
                     to_add = static_cast<int64_t>(effective_csaf_per_account * score_earned.value / total_award_csaf);
                  const auto& score_obj = get(std::get<0>(e));
                  modify(score_obj, [&](score_object& obj)
                  {
                     obj.profits = to_add;
                  });

                  //registrar and referrer get part of earning
                  if (dpo.enabled_hardfork_version >= ENABLE_HEAD_FORK_05)
                  {
                     share_type to_registrar_and_referrer = static_cast<int64_t>((uint128_t)to_add.value * params.registrar_referrer_rate_from_score / GRAPHENE_100_PERCENT);
                     registrar_and_referrer_award[score_obj.from_account_uid] += to_registrar_and_referrer;
                     adjust_balance_map[score_obj.from_account_uid] += (to_add - to_registrar_and_referrer);
                  }
                  else
                     adjust_balance_map[score_obj.from_account_uid] += to_add;

                  actual_score_earned += to_add;
               }

               modify(*(std::get<0>(*itr)), [&](active_post_object& act)
               {
                  act.post_award = actual_score_earned + receiptor_earned;
               });

               modify(post, [&](post_object& act)
               {
                  act.score_settlement = true;
               });
            }

            for (const auto& p : platform_receiptor_award)
            {
               ///adjust_balance(p.first, asset(p.second.first));
               if (auto platform = find_platform_by_owner(p.first))
               {
                  modify(*platform, [&](platform_object& pla)
                  {
                     pla.add_period_profits(
                        dpo.current_active_post_sequence,
                        _latest_active_post_periods,
                        asset(),
                        0,
                        p.second.first,
                        0,
                        p.second.second);
                  });
               }
            }

            //registrar and referrer bonus from score earning
            map<account_uid_type, share_type> bonus_map;
            for (const auto& r : registrar_and_referrer_award)
            {
               auto account_obj = get_account_by_uid(r.first);
               share_type to_registrar = static_cast<int64_t>((uint128_t)r.second.value * account_obj.reg_info.registrar_percent
                  / GRAPHENE_100_PERCENT);
               bonus_map[account_obj.reg_info.registrar] += to_registrar;
               bonus_map[account_obj.reg_info.referrer] += (r.second - to_registrar);
            }
            for (const auto& r : bonus_map)
            {
               modify(get_account_statistics_by_uid(r.first), [&](_account_statistics_object& s)
               {
                  s.uncollected_score_bonus += r.second;
               });
               actual_awards += r.second;
            }
         }

         if (params.total_platform_content_award_amount > 0 && total_csaf_amount > 0)
         {
            //compute per period award amount 
            uint128_t content_platform_award_amount_per_period = (uint128_t)(params.total_content_award_amount.value) *
               (dpo.next_content_award_time - dpo.last_content_award_time).to_seconds() / (86400 * 365);

            for (const auto& p : platform_csaf_amount)
            {
               share_type to_add = static_cast<int64_t>(content_platform_award_amount_per_period * p.second.value /
                  total_csaf_amount.value);
               ///adjust_balance(p.first, asset(to_add));
               adjust_balance_map[p.first] += to_add;

               if (auto platform = find_platform_by_owner(p.first))
               {
                  modify(*platform, [&](platform_object& pla)
                  {
                     pla.add_period_profits(
                        dpo.current_active_post_sequence,
                        _latest_active_post_periods,
                        asset(),
                        0,
                        0,
                        to_add);
                  });
               }
            }
         }

         for (const auto& a : adjust_balance_map)
         {
            actual_awards += a.second;
            adjust_balance(a.first, asset(a.second));
         }
      }

      modify(dpo, [&](dynamic_global_property_object& _dpo)
      {
         _dpo.last_content_award_time = block_time;
         _dpo.next_content_award_time = block_time + params.content_award_interval;
         ++_dpo.current_active_post_sequence;

         if (actual_awards > 0)
            _dpo.budget_pool -= actual_awards;
      });

      clear_active_post();
   }
   else if (dpo.content_award_skip_flag)
   {
      modify(dpo, [&](dynamic_global_property_object& _dpo)
      {
         _dpo.content_award_skip_flag = false;
      });
   }
}

void database::process_platform_voted_awards()
{
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   const auto block_time = head_block_time();
   if (block_time >= dpo.next_platform_voted_award_time)
   {
      const global_property_object& gpo = get_global_properties();
      const auto& params = gpo.parameters.get_extension_params();

      if (params.total_platform_voted_award_amount > 0 && params.platform_award_interval > 0)
      {
         share_type actual_awards = 0; 
         bool can_award = false;
         if (dpo.enabled_hardfork_version >= ENABLE_HEAD_FORK_05)
         {
            fc::uint128_t award_two_periods = (fc::uint128_t)params.total_platform_voted_award_amount.value * 2 *
               (dpo.next_platform_voted_award_time - dpo.last_platform_voted_award_time).to_seconds() / (86400 * 365);
            can_award = dpo.budget_pool >= static_cast<int64_t>(award_two_periods);
         } else {
            can_award = dpo.budget_pool >= params.total_platform_voted_award_amount;
         }

         if (dpo.next_platform_voted_award_time > time_point_sec(0) && can_award)
         {
            flat_map<account_uid_type, uint64_t> platforms;

            uint128_t total_votes = 0;
            const auto& pla_idx = get_index_type<platform_index>().indices().get<by_platform_votes>();
            auto pla_itr = pla_idx.lower_bound(std::make_tuple(true));// assume false < true
            auto limit = params.platform_award_requested_rank;
            while (pla_itr != pla_idx.end() && limit > 0)
            {
               if (pla_itr->total_votes < params.platform_award_min_votes)
                  break;
               //a account only has a platform
               platforms.emplace(pla_itr->owner, pla_itr->total_votes);
               total_votes += pla_itr->total_votes;
               ++pla_itr;
               --limit;
            }
            if (platforms.size() > 0)
            {
               //compute per period award amount 
               uint128_t value = (uint128_t)(params.total_platform_voted_award_amount.value) *
                  (dpo.next_platform_voted_award_time - dpo.last_platform_voted_award_time).to_seconds() / (86400 * 365);

               share_type platform_award_basic = static_cast<int64_t>(value * params.platform_award_basic_rate / GRAPHENE_100_PERCENT);
               share_type platform_average_award_basic = platform_award_basic / platforms.size();
               flat_map<account_uid_type, share_type> platform_award;
               for (const auto& p : platforms)
                  platform_award.emplace(p.first, platform_average_award_basic);
               actual_awards = platform_average_award_basic * platforms.size();

               if (total_votes > 0)
               {
                  share_type platform_award_by_votes = static_cast<int64_t>(value) - platform_award_basic;
                  for (const auto& p : platforms)
                  {
                     share_type to_add = static_cast<int64_t>((uint128_t)platform_award_by_votes.value * p.second / total_votes);
                     actual_awards += to_add;
                     platform_award.at(p.first) += to_add;
                  }
               }

               for (const auto& p : platform_award)
               {
                  adjust_balance(p.first, asset(p.second));
                  const auto& platform = get_platform_by_owner(p.first);
                  modify(platform, [&](platform_object& pla)
                  {
                     if (pla.vote_profits.size() >= _latest_active_post_periods)
                        pla.vote_profits.erase(pla.vote_profits.begin());
                     pla.vote_profits.emplace(block_time, p.second);
                  });
               }
            }
         }

         modify(dpo, [&](dynamic_global_property_object& _dpo)
         {
            _dpo.last_platform_voted_award_time = block_time;
            _dpo.next_platform_voted_award_time = block_time + params.platform_award_interval;

            if (actual_awards > 0)
               _dpo.budget_pool -= actual_awards;
         });

      }
      else if (dpo.next_platform_voted_award_time != time_point_sec(0))
      {
         modify(dpo, [&](dynamic_global_property_object& _dpo)
         {
            _dpo.last_platform_voted_award_time = time_point_sec(0);
            _dpo.next_platform_voted_award_time = time_point_sec(0);
         });
      }
   }
}

void database::process_pledge_balance_release()
{
   const auto head_num = head_block_num();
   const auto& pledge_idx = get_index_type<pledge_balance_index>().indices().get<by_earliest_release_block_number>();

   //release pledge balance 
   auto itr_pledge = pledge_idx.begin();
   while (itr_pledge != pledge_idx.end() && itr_pledge->earliest_release_block_number() <= head_num)
   {
      const dynamic_global_property_object& dpo = get_dynamic_global_properties();
      if (dpo.enabled_hardfork_version == ENABLE_HEAD_FORK_04 && itr_pledge->type == pledge_balance_type::Witness){
         const uint64_t csaf_window = get_global_properties().parameters.csaf_accumulate_window;
         modify(get_account_statistics_by_uid(itr_pledge->superior_index), [&](_account_statistics_object& s) {
            s.update_coin_seconds_earned(csaf_window, head_block_time(), *this, ENABLE_HEAD_FORK_04);
         });
      }

      FC_ASSERT(itr_pledge->pledge >= 0, "pledge_balance_object`s pledge must >= 0. ");
      modify(*itr_pledge, [&](pledge_balance_object& s) {
         share_type delta = 0;
         auto itr = s.releasing_pledges.begin();
         while (itr != s.releasing_pledges.end() && itr->first <= head_num){
            FC_ASSERT(s.total_releasing_pledge >= itr->second, "total_releasing_pledge must more than single pledge. \n pledge_balance_object`s detail:${d}", ("d",s));
            s.total_releasing_pledge -= itr->second;
            delta += itr->second;
            s.releasing_pledges.erase(itr);
            itr = s.releasing_pledges.begin();
         }
         if (itr_pledge->type == pledge_balance_type::Mine){
            account_uid_type pledge_miner = get(pledge_mining_id_type(itr_pledge->superior_index)).pledge_account;
            modify(get_account_statistics_by_uid(pledge_miner), [&](_account_statistics_object& s) {
               s.total_mining_pledge -= delta;
            });
         }
      });

      if (itr_pledge->pledge == 0 && itr_pledge->releasing_pledges.size() == 0){
         if (itr_pledge->type == pledge_balance_type::Mine){
            remove(get(pledge_mining_id_type(itr_pledge->superior_index)));
         }
         else{
            const _account_statistics_object& ant = get_account_statistics_by_uid(itr_pledge->superior_index);
            modify(ant, [&](_account_statistics_object& a){
               a.pledge_balance_ids.erase(itr_pledge->type);
            });
         }
         remove(*itr_pledge);
      }

      itr_pledge = pledge_idx.begin();
   }
}

} }
