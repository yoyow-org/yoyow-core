/*
 * Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
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

   const auto head_time = d.head_block_time();
   const uint64_t csaf_window = global_params.csaf_accumulate_window;

   FC_ASSERT( op.time <= head_time, "Time should not be later than head block time" );
   FC_ASSERT( op.time + GRAPHENE_MAX_CSAF_COLLECTING_TIME_OFFSET >= head_time,
              "Time should not be earlier than 5 minutes before head block time" );

   available_coin_seconds = from_stats->compute_coin_seconds_earned( csaf_window, op.time ).first;

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
      s.set_coin_seconds_earned( available_coin_seconds - collecting_coin_seconds, o.time );
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
      FC_ASSERT( op.amount.amount > 0, "Should lease something" );
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
      auto available_balance = from_stats->core_balance
                             - from_stats->core_leased_out
                             - from_stats->total_witness_pledge
                             - from_stats->total_platform_pledge
                             - from_stats->total_committee_member_pledge;
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

   if( delta != 0 )
   {
      const auto head_time = d.head_block_time();
      const uint64_t csaf_window = d.get_global_properties().parameters.csaf_accumulate_window;
      bool reduce_witness = d.head_block_time() > HARDFORK_0_4_TIME;
      d.modify( *from_stats, [&](account_statistics_object& s) {
         s.update_coin_seconds_earned(csaf_window, head_time, reduce_witness);
         s.core_leased_out += delta;
      });
      d.modify( *to_stats, [&](account_statistics_object& s) {
         s.update_coin_seconds_earned(csaf_window, head_time, reduce_witness);
         s.core_leased_in += delta;
      });
   }

   return return_result;

} FC_CAPTURE_AND_RETHROW( (o) ) }


} } // graphene::chain
