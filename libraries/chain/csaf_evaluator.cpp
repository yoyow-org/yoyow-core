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
#include <graphene/chain/csaf_evaluator.hpp>
#include <graphene/chain/csaf_object.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>

namespace graphene { namespace chain {

void_result csaf_collect_evaluator::do_evaluate( const csaf_collect_operation& op )
{ try {

   const database& d = db();

   from_stats = &d.get_account_statistics_by_uid( op.from );
   to_stats = &d.get_account_statistics_by_uid( op.to );

   const auto& global_params = d.get_global_properties().parameters;

   FC_ASSERT( op.amount.amount + to_stats->csaf <= global_params.max_csaf_per_account,
              "Maximum CSAF per account exceeded" );

   const uint64_t csaf_window = global_params.csaf_accumulate_window;

   available_coin_seconds = from_stats->compute_coin_seconds_earned( csaf_window, d.head_block_time() ).first;

   collecting_coin_seconds = fc::uint128_t(op.amount.amount.value) * global_params.csaf_rate;

   FC_ASSERT( available_coin_seconds >= collecting_coin_seconds,
              "Insufficient CSAF: account ${a}'s available CSAF of ${b} is less than required ${r}",
              ("a",op.from)
              ("b",d.to_pretty_core_string((available_coin_seconds / global_params.csaf_rate).to_uint64()))
              ("r",d.to_pretty_string(op.amount)) );

   return void_result();

}  FC_CAPTURE_AND_RETHROW( (op) ) }

void_result csaf_collect_evaluator::do_apply( const csaf_collect_operation& o )
{ try {
   database& d = db();

   d.modify( *from_stats, [&](account_statistics_object& s) {
      s.set_coin_seconds_earned( available_coin_seconds - collecting_coin_seconds, d.head_block_time() );
   });

   d.modify( *to_stats, [&](account_statistics_object& s) {
      s.csaf += o.amount.amount;
   });

   return void_result();

} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result csaf_lease_evaluator::do_evaluate( const csaf_lease_operation& op )
{ try {

   const database& d = db();

   FC_ASSERT( op.amount.amount == 0 || op.expiration > d.head_block_time(), "CSAF lease should expire later" );

   auto& idx = d.get_index_type<csaf_lease_index>().indices().get<by_from_to>();
   auto  itr = idx.find( boost::make_tuple( op.from, op.to ) );
   if( itr == idx.end() )
   {
      FC_ASSERT( op.amount.amount != 0, "Should lease something" );
      delta = op.amount.amount;
   }
   else
   {
      FC_ASSERT( itr->amount != op.amount.amount || itr->expiration != op.expiration, "Should change something" );
      delta = op.amount.amount - itr->amount;
      current_lease = &(*itr);
   }

   from_stats = &d.get_account_statistics_by_uid( op.from );
   to_stats = &d.get_account_statistics_by_uid( op.to );

   if( delta > 0 )
   {
      auto available_balance = from_stats->core_balance - from_stats->core_leased_out - from_stats->total_witness_pledge;
      FC_ASSERT( available_balance >= delta,
                 "Insufficient Balance: account ${a}'s available balance of ${b} is less than required ${r}",
                 ("a",op.from)
                 ("b",d.to_pretty_core_string(available_balance))
                 ("r",d.to_pretty_core_string(delta)) );
   }

   return void_result();

}  FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type csaf_lease_evaluator::do_apply( const csaf_lease_operation& o )
{ try {
   database& d = db();
   object_id_type return_result;
   if( current_lease == nullptr )
   {
      current_lease = &d.create<csaf_lease_object>([&](csaf_lease_object& clo) {
         clo.from       = o.from;
         clo.to         = o.to;
         clo.amount     = o.amount.amount;
         clo.expiration = o.expiration;
      });
      return_result = current_lease->id;
   }
   else if( o.amount.amount > 0 )
   {
      d.modify( *current_lease, [&](csaf_lease_object& clo) {
         clo.amount     = o.amount.amount;
         clo.expiration = o.expiration;
      });
      return_result = current_lease->id;
   }
   else // o.amount.amount == 0
   {
      return_result = current_lease->id;
      d.remove( *current_lease );
   }

   const uint64_t csaf_window = d.get_global_properties().parameters.csaf_accumulate_window;
   d.modify( *from_stats, [&](account_statistics_object& s) {
      s.update_coin_seconds_earned( csaf_window, d.head_block_time() );
      s.core_leased_out += delta;
   });
   d.modify( *to_stats, [&](account_statistics_object& s) {
      s.update_coin_seconds_earned( csaf_window, d.head_block_time() );
      s.core_leased_in += delta;
   });

   return return_result;

} FC_CAPTURE_AND_RETHROW( (o) ) }


} } // graphene::chain
