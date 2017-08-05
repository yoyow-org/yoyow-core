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
#include <graphene/chain/global_property_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/witness_schedule_object.hpp>

namespace graphene { namespace chain {

using boost::container::flat_set;

account_uid_type database::get_scheduled_witness( uint32_t slot_num )const
{
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   const witness_schedule_object& wso = witness_schedule_id_type()(*this);
   uint64_t current_aslot = dpo.current_aslot + slot_num;
   return wso.current_shuffled_witnesses[ current_aslot % wso.current_shuffled_witnesses.size() ];
}

fc::time_point_sec database::get_slot_time(uint32_t slot_num)const
{
   if( slot_num == 0 )
      return fc::time_point_sec();

   auto interval = block_interval();
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();

   if( head_block_num() == 0 )
   {
      // n.b. first block is at genesis_time plus one block interval
      fc::time_point_sec genesis_time = dpo.time;
      return genesis_time + slot_num * interval;
   }

   int64_t head_block_abs_slot = head_block_time().sec_since_epoch() / interval;
   fc::time_point_sec head_slot_time(head_block_abs_slot * interval);

   const global_property_object& gpo = get_global_properties();

   if( dpo.dynamic_flags & dynamic_global_property_object::maintenance_flag )
      slot_num += gpo.parameters.maintenance_skip_slots;

   // "slot 0" is head_slot_time
   // "slot 1" is head_slot_time,
   //   plus maint interval if head block is a maint block
   //   plus block interval if head block is not a maint block
   return head_slot_time + (slot_num * interval);
}

uint32_t database::get_slot_at_time(fc::time_point_sec when)const
{
   fc::time_point_sec first_slot_time = get_slot_time( 1 );
   if( when < first_slot_time )
      return 0;
   return (when - first_slot_time).to_seconds() / block_interval() + 1;
}

uint32_t database::witness_participation_rate()const
{
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   return uint64_t(GRAPHENE_100_PERCENT) * dpo.recent_slots_filled.popcount() / 128;
}

void database::update_witness_schedule()
{
   const witness_schedule_object& wso = witness_schedule_id_type()(*this);
   const global_property_object& gpo = get_global_properties();

   if( head_block_num() >= wso.next_schedule_block_num )
   {
      modify( wso, [&]( witness_schedule_object& _wso )
      {
         _wso.current_shuffled_witnesses.clear();
         _wso.current_shuffled_witnesses.reserve( gpo.active_witnesses.size() );

         for( const account_uid_type& w : gpo.active_witnesses )
            _wso.current_shuffled_witnesses.push_back( w );

         auto now_hi = uint64_t(head_block_time().sec_since_epoch()) << 32;
         for( uint32_t i = 0; i < _wso.current_shuffled_witnesses.size(); ++i )
         {
            /// High performance random generator
            /// http://xorshift.di.unimi.it/
            uint64_t k = now_hi + uint64_t(i)*2685821657736338717ULL;
            k ^= (k >> 12);
            k ^= (k << 25);
            k ^= (k >> 27);
            k *= 2685821657736338717ULL;

            uint32_t jmax = _wso.current_shuffled_witnesses.size() - i;
            uint32_t j = i + k%jmax;
            std::swap( _wso.current_shuffled_witnesses[i],
                       _wso.current_shuffled_witnesses[j] );
         }
         _wso.next_schedule_block_num += _wso.current_shuffled_witnesses.size();
      });
      dlog( "witness schedule updated on block ${n}, next reschedule block is ${b}",
            ("n",head_block_num())("b",wso.next_schedule_block_num) );
   }
}

void database::update_witness_avg_pledge( const account_uid_type uid )
{
   update_witness_avg_pledge( get_witness_by_uid( uid ) );
}

void database::update_witness_avg_pledge( const witness_object& wit )
{
   const auto& global_params = get_global_properties().parameters;
   const auto window = global_params.max_witness_pledge_seconds;
   const auto now = head_block_time();

   // update position
   const witness_schedule_object& wso = witness_schedule_id_type()(*this);
   if( wso.current_by_pledge_time > wit.by_pledge_position_last_update )
   {
      modify( wit, [&]( witness_object& w )
      {
         auto delta_pos = (wso.current_by_pledge_time - w.by_pledge_position_last_update) * w.average_pledge;
         w.by_pledge_position += delta_pos;
         w.by_pledge_position_last_update = wso.current_by_pledge_time;
      } );
   }

   // update avg pledge
   const auto old_avg_pledge = wit.average_pledge;
   if( wit.average_pledge == wit.pledge )
   {
      modify( wit, [&]( witness_object& w )
      {
         w.average_pledge_last_update = now;
         w.average_pledge_next_update_block = -1;
      } );
   }
   else if( wit.average_pledge > wit.pledge || now >= wit.pledge_last_update + window )
   {
      modify( wit, [&]( witness_object& w )
      {
         w.average_pledge = w.pledge;
         w.average_pledge_last_update = now;
         w.average_pledge_next_update_block = -1;
      } );
   }
   else if( now > wit.average_pledge_last_update )
   {
      // need to schedule next update because average_pledge < pledge, and need to update average_pledge
      uint64_t delta_seconds = ( now - wit.average_pledge_last_update ).to_seconds();
      uint64_t old_seconds = window - delta_seconds;

      fc::uint128_t old_coin_seconds = fc::uint128_t( wit.average_pledge ) * old_seconds;
      fc::uint128_t new_coin_seconds = fc::uint128_t( wit.pledge ) * delta_seconds;

      uint64_t new_average_coins = ( ( old_coin_seconds + new_coin_seconds ) / window ).to_uint64();

      modify( wit, [&]( witness_object& w )
      {
         w.average_pledge = new_average_coins;
         w.average_pledge_last_update = now;
         w.average_pledge_next_update_block = head_block_num() + global_params.witness_avg_pledge_update_interval;
      } );
   }
   else
   {
      // need to schedule next update because average_pledge < pledge, but no need to update average_pledge
      modify( wit, [&]( witness_object& w )
      {
         w.average_pledge_next_update_block = head_block_num() + global_params.witness_avg_pledge_update_interval;
      } );
   }

   // update scheduled time
   if( old_avg_pledge != wit.average_pledge )
   {
      modify( wit, [&]( witness_object& w )
      {
         const auto need_time = ( GRAPHENE_VIRTUAL_LAP_LENGTH - w.by_pledge_position ) / ( w.average_pledge + 1 );
         w.by_pledge_scheduled_time = w.by_pledge_position_last_update + need_time;
         // check for overflow
         if( w.by_pledge_scheduled_time < wso.current_by_pledge_time )
            w.by_pledge_scheduled_time = fc::uint128_t::max_value();
      } );
   }
}

void database::reset_witness_by_pledge_schedule()
{
   const witness_schedule_object& wso = witness_schedule_id_type()(*this);
   modify( wso, [&](witness_schedule_object& o )
   {
       o.current_by_pledge_time = fc::uint128_t(); // reset it to 0
   } );

   const auto& idx = get_index_type<witness_index>().indices();
   for( const auto& witness : idx )
   {
      modify( witness, [&]( witness_object& w )
      {
         w.by_pledge_position             = fc::uint128_t();
         w.by_pledge_position_last_update = fc::uint128_t();
         w.by_pledge_scheduled_time       = GRAPHENE_VIRTUAL_LAP_LENGTH / ( w.average_pledge + 1 );
      } );
   }
}

void database::reset_witness_by_vote_schedule()
{
   const witness_schedule_object& wso = witness_schedule_id_type()(*this);
   modify( wso, [&](witness_schedule_object& o )
   {
       o.current_by_vote_time = fc::uint128_t(); // reset it to 0
   } );

   const auto& idx = get_index_type<witness_index>().indices();
   for( const auto& witness : idx )
   {
      modify( witness, [&]( witness_object& w )
      {
         w.by_vote_position             = fc::uint128_t();
         w.by_vote_position_last_update = fc::uint128_t();
         w.by_vote_scheduled_time       = GRAPHENE_VIRTUAL_LAP_LENGTH / ( w.average_pledge + 1 );
      } );
   }
}

void database::adjust_witness_votes( const witness_object& witness, share_type delta )
{
   if( delta == 0 )
      return;

   const witness_schedule_object& wso = witness_schedule_id_type()(*this);
   modify( witness, [&]( witness_object& w )
   {
      // update position
      if( wso.current_by_vote_time > w.by_vote_position_last_update )
      {
         auto delta_pos = (wso.current_by_vote_time - w.by_vote_position_last_update) * w.total_votes;
         w.by_vote_position += delta_pos;
         w.by_vote_position_last_update = wso.current_by_vote_time;
      }

      w.total_votes += delta.value;

      const auto need_time = ( GRAPHENE_VIRTUAL_LAP_LENGTH - w.by_vote_position ) / ( w.total_votes + 1 );
      w.by_vote_scheduled_time = w.by_vote_position_last_update + need_time;
      // check for overflow
      if( w.by_vote_scheduled_time < wso.current_by_vote_time )
         w.by_vote_scheduled_time = fc::uint128_t::max_value();
   } );
}


} }
