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
#include <graphene/chain/content_evaluator.hpp>
#include <graphene/chain/content_object.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>

namespace graphene { namespace chain {

void_result platform_create_evaluator::do_evaluate( const platform_create_operation& op )
{ try {
   database& d = db();
   
   account_stats = &d.get_account_statistics_by_uid( op.account );
   account_obj = &d.get_account_by_uid( op.account );

   const auto& global_params = d.get_global_properties().parameters;
   
   FC_ASSERT( op.pledge.amount >= global_params.platform_min_pledge,
                 "Insufficient pledge: provided ${p}, need ${r}",
                 ("p",d.to_pretty_string(op.pledge))
                 ("r",d.to_pretty_core_string(global_params.platform_min_pledge)) );

   auto available_balance = account_stats->core_balance
                          - account_stats->core_leased_out;
   FC_ASSERT( available_balance >= op.pledge.amount,
              "Insufficient Balance: account ${a}'s available balance of ${b} is less than required ${r}",
              ("a",op.account)
              ("b",d.to_pretty_core_string(available_balance))
              ("r",d.to_pretty_string(op.pledge)) );

   const platform_object* maybe_found = d.find_platform_by_uid( op.account );
   FC_ASSERT( maybe_found == nullptr, "This account already has a platform" );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type platform_create_evaluator::do_apply( const platform_create_operation& op )
{ try {
   database& d = db();

   const auto& new_platform_object = d.create<platform_object>( [&]( platform_object& pf ){
      pf.owner                = op.account;
      pf.name                 = op.name;
      pf.sequence            = account_stats->last_platform_sequence + 1;
      pf.pledge              = op.pledge.amount.value;
      pf.url                 = op.url;
      pf.extra_data          = op.extra_data;
      pf.create_time         = d.head_block_time();

   });

   d.modify( *account_stats, [&](account_statistics_object& s) {
      s.last_platform_sequence += 1;
      if( s.releasing_platform_pledge > op.pledge.amount.value )
         s.releasing_platform_pledge -= op.pledge.amount.value;
      else
      {
         s.total_platform_pledge = op.pledge.amount.value;
         if( s.releasing_platform_pledge > 0 )
         {
            s.releasing_platform_pledge = 0;
            s.platform_pledge_release_block_number = -1;
         }
      }
   });

   return new_platform_object.id;
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result platform_update_evaluator::do_evaluate( const platform_update_operation& op )
{ try {
   database& d = db();
   account_stats = &d.get_account_statistics_by_uid( op.account );
   platform_obj   = &d.get_platform_by_uid( op.account );

   const auto& global_params = d.get_global_properties().parameters;

   if( op.new_pledge.valid() )
   {
      if( op.new_pledge->amount > 0 ) // change pledge
      {
         FC_ASSERT( op.new_pledge->amount >= global_params.platform_min_pledge,
                    "Insufficient pledge: provided ${p}, need ${r}",
                    ("p",d.to_pretty_string(*op.new_pledge))
                    ("r",d.to_pretty_core_string(global_params.platform_min_pledge)) );

         auto available_balance = account_stats->core_balance
                                - account_stats->core_leased_out;
         FC_ASSERT( available_balance >= op.new_pledge->amount,
                    "Insufficient Balance: account ${a}'s available balance of ${b} is less than required ${r}",
                    ("a",op.account)
                    ("b",d.to_pretty_core_string(available_balance))
                    ("r",d.to_pretty_string(*op.new_pledge)) );
      }
      else if( op.new_pledge->amount == 0 ) // resign
      {
         
      }
   }
   else // 更新平台时，必需检查抵押
   {
      FC_ASSERT( platform_obj->pledge >= global_params.platform_min_pledge,
                 "Insufficient pledge: has ${p}, need ${r}",
                 ("p",d.to_pretty_core_string(platform_obj->pledge))
                 ("r",d.to_pretty_core_string(global_params.platform_min_pledge)) );
   }

   if( op.new_url.valid() )
      FC_ASSERT( *op.new_url != platform_obj->url, "new_url specified but did not change" );

   if( op.new_name.valid() )
      FC_ASSERT( *op.new_name != platform_obj->name, "new_name specified but did not change" );

   if( op.new_extra_data.valid() )
      FC_ASSERT( *op.new_extra_data != platform_obj->extra_data, "new_extra_data specified but did not change" );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result platform_update_evaluator::do_apply( const platform_update_operation& op )
{ try {
   database& d = db();

   const auto& global_params = d.get_global_properties().parameters;

   if( !op.new_pledge.valid() ) // change url or name or extra_data
   {
      d.modify( *platform_obj, [&]( platform_object& pfo ) {
         if( op.new_name.valid() )
            pfo.name = *op.new_name;
         if( op.new_url.valid() )
            pfo.url = *op.new_url;
         if( op.new_extra_data.valid())
            pfo.extra_data = *op.new_extra_data;
      });
   }
   else if( op.new_pledge->amount == 0 ) // resign
   {
      d.modify( *account_stats, [&](account_statistics_object& s) {
         s.releasing_platform_pledge = s.total_platform_pledge;
         s.platform_pledge_release_block_number = d.head_block_num() + global_params.platform_pledge_release_delay;
      });
      d.modify( *platform_obj, [&]( platform_object& pfo ) {
         pfo.is_valid = false; // 将延迟处理
      });
   }
   else // change pledge
   {
      // update account stats
      share_type delta = op.new_pledge->amount.value - platform_obj->pledge;
      if( delta > 0 ) // 增加抵押
      {
         d.modify( *account_stats, [&](account_statistics_object& s) {
            if( s.releasing_platform_pledge > delta )
               s.releasing_platform_pledge -= delta.value;
            else
            {
               s.total_platform_pledge = op.new_pledge->amount.value;
               if( s.releasing_platform_pledge > 0 )
               {
                  s.releasing_platform_pledge = 0;
                  s.platform_pledge_release_block_number = -1;
               }
            }
         });
      }
      else // 减少抵押
      {
         d.modify( *account_stats, [&](account_statistics_object& s) {
            s.releasing_platform_pledge -= delta.value;
            s.platform_pledge_release_block_number = d.head_block_num() + global_params.platform_pledge_release_delay;
         });
      }

      // update platform data
      d.modify( *platform_obj, [&]( platform_object& pfo ) {
         if( op.new_name.valid() )
            pfo.name = *op.new_name;
         if( op.new_url.valid() )
            pfo.url = *op.new_url;
         if( op.new_extra_data.valid() )
            pfo.extra_data = *op.new_extra_data;

         pfo.pledge              = op.new_pledge->amount.value;
         pfo.last_update_time  = d.head_block_time();

      });
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result platform_vote_update_evaluator::do_evaluate( const platform_vote_update_operation& op )
{ try {
   database& d = db();
   account_stats = &d.get_account_statistics_by_uid( op.voter );

   FC_ASSERT( account_stats->can_vote == true, "This account can not vote" );

   const auto& global_params = d.get_global_properties().parameters;
   FC_ASSERT( account_stats->core_balance >= global_params.min_governance_voting_balance,
              "Need more balance to be able to vote: have ${b}, need ${r}",
              ("b",d.to_pretty_core_string(account_stats->core_balance))
              ("r",d.to_pretty_core_string(global_params.min_governance_voting_balance)) );

   const auto max_platforms = global_params.platform_max_vote_per_account;
   FC_ASSERT( op.platform_to_add.size() <= max_platforms,
              "Trying to vote for ${n} platforms, more than allowed maximum: ${m}",
              ("n",op.platform_to_add.size())("m",max_platforms) );

   for( const auto uid : op.platform_to_remove )
   {
      const platform_object& po = d.get_platform_by_uid( uid );
      platform_to_remove.push_back( &po );
   }
   for( const auto uid : op.platform_to_add )
   {
      const platform_object& po = d.get_platform_by_uid( uid );
      platform_to_add.push_back( &po );
   }

   if( account_stats->is_voter ) // maybe valid voter
   {
      voter_obj = d.find_voter( op.voter, account_stats->last_voter_sequence );
      FC_ASSERT( voter_obj != nullptr, "voter should exist" );

      // check if the voter is still valid
      bool still_valid = d.check_voter_valid( *voter_obj, true );
      if( !still_valid )
      {
         invalid_voter_obj = voter_obj;
         voter_obj = nullptr;
      }
   }
   // else do nothing

   if( voter_obj == nullptr ) // not voting
      FC_ASSERT( op.platform_to_remove.empty(), "Not voting for any platform, or votes were no longer valid, can not remove" );
   else if( voter_obj->proxy_uid != GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID ) // voting with a proxy
   {
      // check if the proxy is still valid
      const voter_object* current_proxy_voter_obj = d.find_voter( voter_obj->proxy_uid, voter_obj->proxy_sequence );
      FC_ASSERT( current_proxy_voter_obj != nullptr, "proxy voter should exist" );
      bool current_proxy_still_valid = d.check_voter_valid( *current_proxy_voter_obj, true );
      if( current_proxy_still_valid ) // still valid
         FC_ASSERT( op.platform_to_remove.empty() && op.platform_to_add.empty(),
                    "Now voting with a proxy, can not add or remove platform" );
      else // no longer valid
      {
         invalid_current_proxy_voter_obj = current_proxy_voter_obj;
         FC_ASSERT( op.platform_to_remove.empty(),
                    "Was voting with a proxy but it is now invalid, so not voting for any platform, can not remove" );
      }
   }
   else // voting by self
   {
      // check for voted platforms whom have became invalid
      uint16_t platforms_voted = voter_obj->number_of_platform_voted;
      const auto& idx = d.get_index_type<platform_vote_index>().indices().get<by_platform_voter_seq>();
      auto itr = idx.lower_bound( std::make_tuple( op.voter, voter_obj->sequence ) );
      while( itr != idx.end() && itr->voter_uid == op.voter && itr->voter_sequence == voter_obj->sequence )
      {
         const platform_object* platform = d.find_platform_by_uid( itr->platform_uid );
         if( platform == nullptr || platform->sequence != itr->platform_sequence )
         {
            invalid_platform_votes_to_remove.push_back( &(*itr) );
            --platforms_voted;
         }
         ++itr;
      }

      FC_ASSERT( op.platform_to_remove.size() <= platforms_voted,
                 "Trying to remove ${n} platforms, more than voted: ${m}",
                 ("n",op.platform_to_remove.size())("m",platforms_voted) );
      uint16_t new_total = platforms_voted - op.platform_to_remove.size() + op.platform_to_add.size();
      FC_ASSERT( new_total <= max_platforms,
                 "Trying to vote for ${n} platforms, more than allowed maximum: ${m}",
                 ("n",new_total)("m",max_platforms) );

      for( const auto& pf : platform_to_remove )
      {
         const platform_vote_object* pf_vote = d.find_platform_vote( op.voter, voter_obj->sequence, pf->owner, pf->sequence );
         FC_ASSERT( pf_vote != nullptr, "Not voting for platform ${w}, can not remove", ("w",pf->owner) );
         platform_votes_to_remove.push_back( pf_vote );
      }
      for( const auto& pf : platform_to_add )
      {
         const platform_vote_object* pf_vote = d.find_platform_vote( op.voter, voter_obj->sequence, pf->owner, pf->sequence );
         FC_ASSERT( pf_vote == nullptr, "Already voting for platform ${w}, can not add", ("w",pf->owner) );
      }
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result platform_vote_update_evaluator::do_apply( const platform_vote_update_operation& op )
{ try {
   database& d = db();
   const auto head_block_time = d.head_block_time();
   const auto head_block_num  = d.head_block_num();
   const auto& global_params = d.get_global_properties().parameters;
   const auto max_level = global_params.max_governance_voting_proxy_level;

   if( invalid_current_proxy_voter_obj != nullptr )
      d.invalidate_voter( *invalid_current_proxy_voter_obj );

   if( invalid_voter_obj != nullptr )
      d.invalidate_voter( *invalid_voter_obj );

   share_type total_votes = 0;
   if( voter_obj != nullptr ) // voter already exists
   {
      // clear proxy votes
      if( invalid_current_proxy_voter_obj != nullptr )
      {
         d.clear_voter_proxy_votes( *voter_obj );
         // update proxy
         d.modify( *invalid_current_proxy_voter_obj, [&](voter_object& v) {
            v.proxied_voters -= 1;
         });
      }

      // remove invalid platform votes
      for( const auto& pla_vote : invalid_platform_votes_to_remove )
         d.remove( *pla_vote );

      // remove requested platform votes
      total_votes = voter_obj->total_votes();
      const size_t total_remove = platform_to_remove.size();
      for( uint16_t i = 0; i < total_remove; ++i )
      {
         //d.adjust_platform_votes( *platform_to_remove[i], -total_votes );
         d.remove( *platform_votes_to_remove[i] );
      }

      d.modify( *voter_obj, [&](voter_object& v) {
         // update voter proxy to self
         if( invalid_current_proxy_voter_obj != nullptr )
         {
            v.proxy_uid      = GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID;
            v.proxy_sequence = 0;
         }
         v.proxy_last_vote_block[0]  = head_block_num;
         v.effective_last_vote_block = head_block_num;
         v.number_of_platform_voted = v.number_of_platform_voted
                                     - invalid_platform_votes_to_remove.size()
                                     - platform_to_remove.size()
                                     + platform_to_add.size();
      });
   }
   else // need to create a new voter object for this account
   {
      d.modify( *account_stats, [&](account_statistics_object& s) {
         s.is_voter = true;
         s.last_voter_sequence += 1;
      });

      voter_obj = &d.create<voter_object>( [&]( voter_object& v ){
         v.uid               = op.voter;
         v.sequence          = account_stats->last_voter_sequence;
         v.votes             = account_stats->core_balance.value;
         v.votes_last_update = head_block_time;

         v.effective_votes_last_update       = head_block_time;
         v.effective_votes_next_update_block = head_block_num + global_params.governance_votes_update_interval;

         v.proxy_uid         = GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID;
         // v.proxy_sequence = 0; // default

         //v.proxied_voters    = 0; // default
         v.proxied_votes.resize( max_level, 0 ); // [ level1, level2, ... ]
         v.proxy_last_vote_block.resize( max_level+1, 0 ); // [ self, proxy, proxy->proxy, ... ]
         v.proxy_last_vote_block[0] = head_block_num;

         v.effective_last_vote_block         = head_block_num;

         v.number_of_platform_voted         = platform_to_add.size();
      });
   }

   // add requested platform votes
   const size_t total_add = platform_to_add.size();
   for( uint16_t i = 0; i < total_add; ++i )
   {
      d.create<platform_vote_object>( [&]( platform_vote_object& o ){
         o.voter_uid        = op.voter;
         o.voter_sequence   = voter_obj->sequence;
         o.platform_uid      = platform_to_add[i]->owner;
         o.platform_sequence = platform_to_add[i]->sequence;
      });
      //if( total_votes > 0 )
         //d.adjust_platform_votes( *platform_to_add[i], total_votes );
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }



void_result post_evaluator::do_evaluate( const post_operation& op )
{ try {

   const database& d = db();

   d.get_platform_by_uid( op.platform ); // make sure pid exists
   poster_account = &d.get_account_by_uid( op.poster );

   if( op.parent_post_pid.valid() )
      FC_ASSERT( poster_account->can_reply, "poster ${uid} is not allowed to reply.", ("uid",op.poster) );
   else
      FC_ASSERT( poster_account->can_post, "poster ${uid} is not allowed to post.", ("uid",op.poster) );

   post = d.find_post_by_pid( op.platform, op.poster, op.post_pid );

   if( post == nullptr ) // new post
   {
      if( op.parent_post_pid.valid() ) // is reply
         parent_post = &d.get_post_by_pid( op.platform, *op.parent_poster, *op.parent_post_pid );
      else
         parent_post = nullptr;
   }
   else
   {
      FC_ASSERT( !op.parent_post_pid.valid(), "should not specify parent when editing." );
   }

   return void_result();

}  FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type post_evaluator::do_apply( const post_operation& o )
{ try {
   database& d = db();

   if( post == nullptr ) // new post
   {
      const auto& new_post_object = d.create<post_object>( [&]( post_object& obj )
      {
         obj.platform         = o.platform;
         obj.poster           = o.poster;
         obj.post_pid         = o.post_pid;
         obj.parent_poster    = o.parent_poster;
         obj.parent_post_pid  = o.parent_post_pid;
         obj.options          = o.options;
         obj.hash_value       = o.hash_value;
         obj.extra_data       = o.extra_data;
         obj.title            = o.title;
         obj.body             = o.body;
         obj.create_time      = d.head_block_time();
         obj.last_update_time = d.head_block_time();
      } );
      return new_post_object.id;
   }
   else // edit
   {
      d.modify<post_object>( *post, [&]( post_object& obj )
      {
         //obj.options          = o.options;
         obj.hash_value       = o.hash_value;
         obj.extra_data       = o.extra_data;
         obj.title            = o.title;
         obj.body             = o.body;
         obj.last_update_time = d.head_block_time();
      } );
      return post->id;
   }
} FC_CAPTURE_AND_RETHROW( (o) ) }


} } // graphene::chain
