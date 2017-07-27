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
#include <graphene/chain/witness_evaluator.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/protocol/vote.hpp>

namespace graphene { namespace chain {

void_result witness_create_evaluator::do_evaluate( const witness_create_operation& op )
{ try {
   database& d = db();
   //FC_ASSERT(db().get(op.witness_account).is_lifetime_member());
   account_stats = &d.get_account_statistics_by_uid( op.witness_account );

   const auto& global_params = d.get_global_properties().parameters;
   // TODO review
   if( d.head_block_num() > 0 )
      FC_ASSERT( op.pledge.amount >= global_params.min_witness_pledge,
                 "Insufficient pledge: provided ${p}, need ${r}",
                 ("p",d.to_pretty_string(op.pledge))
                 ("r",d.to_pretty_core_string(global_params.min_witness_pledge)) );

   auto available_balance = account_stats->core_balance - account_stats->core_leased_out; // releasing pledge can be reused.
   FC_ASSERT( available_balance >= op.pledge.amount,
              "Insufficient Balance: account ${a}'s available balance of ${b} is less than required ${r}",
              ("a",op.witness_account)
              ("b",d.to_pretty_core_string(available_balance))
              ("r",d.to_pretty_string(op.pledge)) );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type witness_create_evaluator::do_apply( const witness_create_operation& op )
{ try {
   database& d = db();

   const auto& global_params = d.get_global_properties().parameters;

   const auto& new_witness_object = d.create<witness_object>( [&]( witness_object& wit ){
      wit.witness_account     = op.witness_account;
      wit.sequence            = account_stats->last_witness_sequence + 1;
      wit.signing_key         = op.block_signing_key;
      wit.pledge              = op.pledge.amount.value;
      wit.pledge_last_update  = d.head_block_time();
      //wit.vote_id           = vote_id;

      wit.average_pledge_last_update       = d.head_block_time();
      if( wit.pledge > 0 )
         wit.average_pledge_next_update_block = d.head_block_num() + global_params.witness_avg_pledge_update_interval;
      else
         wit.average_pledge_next_update_block = -1;

      wit.url                 = op.url;

   });

   d.modify( *account_stats, [&](account_statistics_object& s) {
      s.last_witness_sequence += 1;
      if( s.releasing_witness_pledge > op.pledge.amount )
         s.releasing_witness_pledge -= op.pledge.amount;
      else
      {
         s.total_witness_pledge = op.pledge.amount;
         if( s.releasing_witness_pledge > 0 )
         {
            s.releasing_witness_pledge = 0;
            s.witness_pledge_release_block_number = -1;
         }
      }
   });

   return new_witness_object.id;
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result witness_update_evaluator::do_evaluate( const witness_update_operation& op )
{ try {
   database& d = db();
   account_stats = &d.get_account_statistics_by_uid( op.witness_account );
   witness_obj   = &d.get_witness_by_uid( op.witness_account );

   const auto& global_params = d.get_global_properties().parameters;
   if( op.new_signing_key.valid() )
      FC_ASSERT( *op.new_signing_key != witness_obj->signing_key, "new_signing_key specified but did not change" );
   if( op.new_pledge.valid() )
   {
      if( op.new_pledge->amount > 0 ) // change pledge
      {
         FC_ASSERT( op.new_pledge->amount >= global_params.min_witness_pledge,
                    "Insufficient pledge: provided ${p}, need ${r}",
                    ("p",d.to_pretty_string(*op.new_pledge))
                    ("r",d.to_pretty_core_string(global_params.min_witness_pledge)) );

         FC_ASSERT( op.new_pledge->amount != witness_obj->pledge, "new_pledge specified but did not change" );

         auto available_balance = account_stats->core_balance - account_stats->core_leased_out; // releasing pledge can be reused.
         FC_ASSERT( available_balance >= op.new_pledge->amount,
                    "Insufficient Balance: account ${a}'s available balance of ${b} is less than required ${r}",
                    ("a",op.witness_account)
                    ("b",d.to_pretty_core_string(available_balance))
                    ("r",d.to_pretty_string(*op.new_pledge)) );
      }
      else if( op.new_pledge->amount == 0 ) // resign
      {
         const auto& active_witnesses = d.get_global_properties().active_witnesses;
         FC_ASSERT( active_witnesses.find( witness_obj->id ) == active_witnesses.end(),
                    "Active witness can not resign" );
      }
   }
   if( op.new_url.valid() )
      FC_ASSERT( *op.new_url != witness_obj->url, "new_url specified but did not change" );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result witness_update_evaluator::do_apply( const witness_update_operation& op )
{ try {
   database& d = db();

   const auto& global_params = d.get_global_properties().parameters;

   if( !op.new_pledge.valid() ) // change url or key
   {
      d.modify( *witness_obj, [&]( witness_object& wit ) {
         if( op.new_signing_key.valid() )
            wit.signing_key = *op.new_signing_key;
         if( op.new_url.valid() )
            wit.url = *op.new_url;
      });
   }
   else if( op.new_pledge->amount == 0 ) // resign
   {
      d.modify( *account_stats, [&](account_statistics_object& s) {
         s.releasing_witness_pledge = s.total_witness_pledge;
         s.witness_pledge_release_block_number = d.head_block_num() + global_params.witness_pledge_release_delay;
      });
      d.remove( *witness_obj );
      // TODO remove votes
   }
   else // change pledge
   {
      // update account stats
      share_type delta = op.new_pledge->amount - witness_obj->pledge;
      if( delta > 0 ) // more pledge
      {
         d.modify( *account_stats, [&](account_statistics_object& s) {
            if( s.releasing_witness_pledge > delta )
               s.releasing_witness_pledge -= delta;
            else
            {
               s.total_witness_pledge = op.new_pledge->amount;
               if( s.releasing_witness_pledge > 0 )
               {
                  s.releasing_witness_pledge = 0;
                  s.witness_pledge_release_block_number = -1;
               }
            }
         });
      }
      else // less pledge
      {
         d.modify( *account_stats, [&](account_statistics_object& s) {
            s.releasing_witness_pledge -= delta;
            s.witness_pledge_release_block_number = d.head_block_num() + global_params.witness_pledge_release_delay;
         });
      }

      // update position
      d.update_witness_avg_pledge( *witness_obj );

      // update witness data
      d.modify( *witness_obj, [&]( witness_object& wit ) {
         if( op.new_signing_key.valid() )
            wit.signing_key = *op.new_signing_key;

         wit.pledge              = op.new_pledge->amount.value;
         wit.pledge_last_update  = d.head_block_time();

         if( op.new_url.valid() )
            wit.url = *op.new_url;
      });

      // update schedule
      d.update_witness_avg_pledge( *witness_obj );
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result witness_vote_update_evaluator::do_evaluate( const witness_vote_update_operation& op )
{ try {
   database& d = db();
   //account_stats = &d.get_account_statistics_by_uid( op.witness_account );
   //witness_obj   = &d.get_witness_by_uid( op.witness_account );

   const auto& global_params = d.get_global_properties().parameters;

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result witness_vote_update_evaluator::do_apply( const witness_vote_update_operation& op )
{ try {
   database& d = db();

   const auto& global_params = d.get_global_properties().parameters;

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result witness_vote_proxy_evaluator::do_evaluate( const witness_vote_proxy_operation& op )
{ try {
   database& d = db();
   //account_stats = &d.get_account_statistics_by_uid( op.witness_account );
   //witness_obj   = &d.get_witness_by_uid( op.witness_account );

   const auto& global_params = d.get_global_properties().parameters;

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result witness_vote_proxy_evaluator::do_apply( const witness_vote_proxy_operation& op )
{ try {
   database& d = db();

   const auto& global_params = d.get_global_properties().parameters;

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

} } // graphene::chain
