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
#include <graphene/chain/committee_member_evaluator.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <graphene/chain/transaction_evaluation_state.hpp>

#include <fc/smart_ref_impl.hpp>

namespace graphene { namespace chain {

void_result committee_member_create_evaluator::do_evaluate( const committee_member_create_operation& op )
{ try {
   database& d = db();
   //FC_ASSERT(db().get(op.account).is_lifetime_member());
   account_stats = &d.get_account_statistics_by_uid( op.account );
   account_obj = &d.get_account_by_uid( op.account );

   const auto& global_params = d.get_global_properties().parameters;
   // don't enforce pledge check for initial committee members
   if( d.head_block_num() > 0 )
      FC_ASSERT( op.pledge.amount >= global_params.min_committee_member_pledge,
                 "Insufficient pledge: provided ${p}, need ${r}",
                 ("p",d.to_pretty_string(op.pledge))
                 ("r",d.to_pretty_core_string(global_params.min_committee_member_pledge)) );

   auto available_balance = account_stats->core_balance
                          - account_stats->core_leased_out
                          - account_stats->total_platform_pledge
                          - account_stats->total_witness_pledge; // releasing committee member pledge can be reused.
   FC_ASSERT( available_balance >= op.pledge.amount,
              "Insufficient Balance: account ${a}'s available balance of ${b} is less than required ${r}",
              ("a",op.account)
              ("b",d.to_pretty_core_string(available_balance))
              ("r",d.to_pretty_string(op.pledge)) );

   const committee_member_object* maybe_found = d.find_committee_member_by_uid( op.account );
   FC_ASSERT( maybe_found == nullptr, "This account is already a committee member" );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type committee_member_create_evaluator::do_apply( const committee_member_create_operation& op )
{ try {
   database& d = db();

   const auto& new_committee_member_object = d.create<committee_member_object>( [&]( committee_member_object& com ){
      com.account             = op.account;
      com.name                = account_obj->name;
      com.sequence            = account_stats->last_committee_member_sequence + 1;
      //com.is_valid            = true; // default
      com.pledge              = op.pledge.amount.value;
      //com.vote_id           = vote_id;

      com.url                 = op.url;

   });

   d.modify( *account_stats, [&](account_statistics_object& s) {
      s.last_committee_member_sequence += 1;
      if( s.releasing_committee_member_pledge > op.pledge.amount )
         s.releasing_committee_member_pledge -= op.pledge.amount;
      else
      {
         s.total_committee_member_pledge = op.pledge.amount;
         if( s.releasing_committee_member_pledge > 0 )
         {
            s.releasing_committee_member_pledge = 0;
            s.committee_member_pledge_release_block_number = -1;
         }
      }
   });

   return new_committee_member_object.id;
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result committee_member_update_evaluator::do_evaluate( const committee_member_update_operation& op )
{ try {
   database& d = db();
   account_stats = &d.get_account_statistics_by_uid( op.account );
   committee_member_obj   = &d.get_committee_member_by_uid( op.account );

   const auto& global_params = d.get_global_properties().parameters;
   if( op.new_pledge.valid() )
   {
      if( op.new_pledge->amount > 0 ) // change pledge
      {
         FC_ASSERT( op.new_pledge->amount >= global_params.min_committee_member_pledge,
                    "Insufficient pledge: provided ${p}, need ${r}",
                    ("p",d.to_pretty_string(*op.new_pledge))
                    ("r",d.to_pretty_core_string(global_params.min_committee_member_pledge)) );

         FC_ASSERT( op.new_pledge->amount != committee_member_obj->pledge, "new_pledge specified but did not change" );

         auto available_balance = account_stats->core_balance
                                - account_stats->core_leased_out
                                - account_stats->total_witness_pledge // releasing committee member pledge can be reused.
                                - account_stats->total_platform_pledge;
         FC_ASSERT( available_balance >= op.new_pledge->amount,
                    "Insufficient Balance: account ${a}'s available balance of ${b} is less than required ${r}",
                    ("a",op.account)
                    ("b",d.to_pretty_core_string(available_balance))
                    ("r",d.to_pretty_string(*op.new_pledge)) );
      }
      else if( op.new_pledge->amount == 0 ) // resign
      {
         // check if we still have enough committee members
         const auto& idx = d.get_index_type<committee_member_index>().indices().get<by_valid>();
         const size_t total_committee_members = idx.count( true );
         FC_ASSERT( total_committee_members > global_params.committee_size,
                    "Need at least ${n} committee members, can not resign at this moment.",
                    ("n", global_params.committee_size) );

      }
   }
   if( op.new_url.valid() )
      FC_ASSERT( *op.new_url != committee_member_obj->url, "new_url specified but did not change" );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result committee_member_update_evaluator::do_apply( const committee_member_update_operation& op )
{ try {
   database& d = db();

   const auto& dpo = d.get_dynamic_global_properties();
   const auto& global_params = d.get_global_properties().parameters;

   if( !op.new_pledge.valid() ) // change url
   {
      d.modify( *committee_member_obj, [&]( committee_member_object& com ) {
         if( op.new_url.valid() )
            com.url = *op.new_url;
      });
   }
   else if( op.new_pledge->amount == 0 ) // resign
   {
      uint32_t pledge_release_block = d.head_block_num() + global_params.committee_member_pledge_release_delay;
      const auto& active_committee_members = d.get_global_properties().active_committee_members;
      if( active_committee_members.find( op.account ) != active_committee_members.end() )
         pledge_release_block = dpo.next_committee_update_block + global_params.committee_member_pledge_release_delay;

      d.modify( *account_stats, [&](account_statistics_object& s) {
         s.releasing_committee_member_pledge = s.total_committee_member_pledge;
         s.committee_member_pledge_release_block_number = pledge_release_block;
      });
      d.modify( *committee_member_obj, [&]( committee_member_object& com ) {
         com.is_valid = false; // will be processed later
      });
   }
   else // change pledge
   {
      // update account stats
      share_type delta = op.new_pledge->amount - committee_member_obj->pledge;
      if( delta > 0 ) // more pledge
      {
         d.modify( *account_stats, [&](account_statistics_object& s) {
            if( s.releasing_committee_member_pledge > delta )
               s.releasing_committee_member_pledge -= delta;
            else
            {
               s.total_committee_member_pledge = op.new_pledge->amount;
               if( s.releasing_committee_member_pledge > 0 )
               {
                  s.releasing_committee_member_pledge = 0;
                  s.committee_member_pledge_release_block_number = -1;
               }
            }
         });
      }
      else // less pledge
      {
         d.modify( *account_stats, [&](account_statistics_object& s) {
            s.releasing_committee_member_pledge -= delta;
            s.committee_member_pledge_release_block_number = d.head_block_num() + global_params.committee_member_pledge_release_delay;
         });
      }

      // update committee_member data
      d.modify( *committee_member_obj, [&]( committee_member_object& com ) {
         com.pledge              = op.new_pledge->amount.value;
         if( op.new_url.valid() )
            com.url = *op.new_url;
      });
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result committee_member_vote_update_evaluator::do_evaluate( const committee_member_vote_update_operation& op )
{ try {
   database& d = db();
   account_stats = &d.get_account_statistics_by_uid( op.voter );

   FC_ASSERT( account_stats->can_vote == true, "This account can not vote" );

   const auto& global_params = d.get_global_properties().parameters;
   FC_ASSERT( account_stats->core_balance >= global_params.min_governance_voting_balance,
              "Need more balance to be able to vote: have ${b}, need ${r}",
              ("b",d.to_pretty_core_string(account_stats->core_balance))
              ("r",d.to_pretty_core_string(global_params.min_governance_voting_balance)) );

   const auto max_committee_members = global_params.max_committee_members_voted_per_account;
   FC_ASSERT( op.committee_members_to_add.size() <= max_committee_members,
              "Trying to vote for ${n} committee_members, more than allowed maximum: ${m}",
              ("n",op.committee_members_to_add.size())("m",max_committee_members) );

   for( const auto uid : op.committee_members_to_remove )
   {
      const committee_member_object& com = d.get_committee_member_by_uid( uid );
      committee_members_to_remove.push_back( &com );
   }
   for( const auto uid : op.committee_members_to_add )
   {
      const committee_member_object& com = d.get_committee_member_by_uid( uid );
      committee_members_to_add.push_back( &com );
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
      FC_ASSERT( op.committee_members_to_remove.empty(),
                 "Not voting for any committee member, or votes were no longer valid, can not remove" );
   else if( voter_obj->proxy_uid != GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID ) // voting with a proxy
   {
      // check if the proxy is still valid
      const voter_object* current_proxy_voter_obj = d.find_voter( voter_obj->proxy_uid, voter_obj->proxy_sequence );
      FC_ASSERT( current_proxy_voter_obj != nullptr, "proxy voter should exist" );
      bool current_proxy_still_valid = d.check_voter_valid( *current_proxy_voter_obj, true );
      if( current_proxy_still_valid ) // still valid
         FC_ASSERT( op.committee_members_to_remove.empty() && op.committee_members_to_add.empty(),
                    "Now voting with a proxy, can not add or remove committee member" );
      else // no longer valid
      {
         invalid_current_proxy_voter_obj = current_proxy_voter_obj;
         FC_ASSERT( op.committee_members_to_remove.empty(),
                    "Was voting with a proxy but it is now invalid, so not voting for any committee member, can not remove" );
      }
   }
   else // voting by self
   {
      // check for voted committee members whom have became invalid
      uint16_t committee_members_voted = voter_obj->number_of_committee_members_voted;
      const auto& idx = d.get_index_type<committee_member_vote_index>().indices().get<by_voter_seq>();
      auto itr = idx.lower_bound( std::make_tuple( op.voter, voter_obj->sequence ) );
      while( itr != idx.end() && itr->voter_uid == op.voter && itr->voter_sequence == voter_obj->sequence )
      {
         const committee_member_object* committee_member = d.find_committee_member_by_uid( itr->committee_member_uid );
         if( committee_member == nullptr || committee_member->sequence != itr->committee_member_sequence )
         {
            invalid_committee_member_votes_to_remove.push_back( &(*itr) );
            --committee_members_voted;
         }
         ++itr;
      }

      FC_ASSERT( op.committee_members_to_remove.size() <= committee_members_voted,
                 "Trying to remove ${n} committee members, more than voted: ${m}",
                 ("n",op.committee_members_to_remove.size())("m",committee_members_voted) );
      uint16_t new_total = committee_members_voted - op.committee_members_to_remove.size() + op.committee_members_to_add.size();
      FC_ASSERT( new_total <= max_committee_members,
                 "Trying to vote for ${n} committee members, more than allowed maximum: ${m}",
                 ("n",new_total)("m",max_committee_members) );

      for( const auto& com : committee_members_to_remove )
      {
         const committee_member_vote_object* com_vote = d.find_committee_member_vote( op.voter, voter_obj->sequence,
                                                                                      com->account, com->sequence );
         FC_ASSERT( com_vote != nullptr, "Not voting for committee_member ${w}, can not remove", ("w",com->account) );
         committee_member_votes_to_remove.push_back( com_vote );
      }
      for( const auto& com : committee_members_to_add )
      {
         const committee_member_vote_object* com_vote = d.find_committee_member_vote( op.voter, voter_obj->sequence,
                                                                                      com->account, com->sequence );
         FC_ASSERT( com_vote == nullptr, "Already voting for committee_member ${w}, can not add", ("w",com->account) );
      }
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result committee_member_vote_update_evaluator::do_apply( const committee_member_vote_update_operation& op )
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

      // remove invalid committee member votes
      for( const auto& com_vote : invalid_committee_member_votes_to_remove )
         d.remove( *com_vote );

      // remove requested committee member votes
      total_votes = voter_obj->total_votes();
      const size_t total_remove = committee_members_to_remove.size();
      for( uint16_t i = 0; i < total_remove; ++i )
      {
         d.adjust_committee_member_votes( *committee_members_to_remove[i], -total_votes );
         d.remove( *committee_member_votes_to_remove[i] );
      }

      d.modify( *voter_obj, [&](voter_object& v) {
         // update voter proxy to self
         if( invalid_current_proxy_voter_obj != nullptr )
         {
            v.proxy_uid      = GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID;
            v.proxy_sequence = 0;
         }
         v.proxy_last_vote_block[0]          = head_block_num;
         v.effective_last_vote_block         = head_block_num;
         v.number_of_committee_members_voted = v.number_of_committee_members_voted
                                             - invalid_committee_member_votes_to_remove.size()
                                             - committee_members_to_remove.size()
                                             + committee_members_to_add.size();
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
         //v.is_valid          = true; // default
         v.votes             = account_stats->core_balance.value;
         v.votes_last_update = head_block_time;

         //v.effective_votes                 = 0; // default
         v.effective_votes_last_update       = head_block_time;
         v.effective_votes_next_update_block = head_block_num + global_params.governance_votes_update_interval;

         v.proxy_uid         = GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID;
         // v.proxy_sequence = 0; // default

         //v.proxied_voters    = 0; // default
         v.proxied_votes.resize( max_level, 0 ); // [ level1, level2, ... ]
         v.proxy_last_vote_block.resize( max_level+1, 0 ); // [ self, proxy, proxy->proxy, ... ]
         v.proxy_last_vote_block[0] = head_block_num;

         v.effective_last_vote_block         = head_block_num;

         v.number_of_committee_members_voted = committee_members_to_add.size();
      });
   }

   // add requested committee_member votes
   const size_t total_add = committee_members_to_add.size();
   for( uint16_t i = 0; i < total_add; ++i )
   {
      d.create<committee_member_vote_object>( [&]( committee_member_vote_object& o ){
         o.voter_uid                 = op.voter;
         o.voter_sequence            = voter_obj->sequence;
         o.committee_member_uid      = committee_members_to_add[i]->account;
         o.committee_member_sequence = committee_members_to_add[i]->sequence;
      });
      if( total_votes > 0 )
         d.adjust_committee_member_votes( *committee_members_to_add[i], total_votes );
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result committee_proposal_create_evaluator::do_evaluate( const committee_proposal_create_operation& op )
{ try {
   database& d = db();

   const auto& gpo = d.get_global_properties();
   const auto& dpo = d.get_dynamic_global_properties();
   const auto& global_params = gpo.parameters;
   const auto& current_committee = gpo.active_committee_members;

   FC_ASSERT( current_committee.find( op.proposer ) != current_committee.end(),
              "Account ${a} is not an active committee member",
              ("a",op.proposer) );

   FC_ASSERT( op.voting_closing_block_num >= d.head_block_num(),
              "Voting closing block number should not be earlier than head block number" );
   FC_ASSERT( op.voting_closing_block_num <= dpo.next_committee_update_block,
              "Voting closing block number should not be later than next committee update block number" );
   FC_ASSERT( op.execution_block_num <= dpo.next_committee_update_block,
              "Proposal execution block number should not be later than next committee update block number" );
   FC_ASSERT( op.expiration_block_num <= dpo.next_committee_update_block,
              "Proposal expiration block number should not be later than next committee update block number" );

   for( const auto& item : op.items )
   {
      if( item.which() == committee_proposal_item_type::tag< committee_update_account_priviledge_item_type >::value )
      {
         const auto& account_item = item.get< committee_update_account_priviledge_item_type >();
         // make sure the accounts exist
         d.get_account_by_uid( account_item.account );
         if( account_item.new_priviledges.value.takeover_registrar.valid() )
            d.get_account_by_uid( *account_item.new_priviledges.value.takeover_registrar );
      }
      else if( item.which() == committee_proposal_item_type::tag< committee_update_global_parameter_item_type >::value )
      {
         const auto& param_item = item.get< committee_update_global_parameter_item_type >();
         if( param_item.value.maximum_time_until_expiration.valid() )
            FC_ASSERT( *param_item.value.maximum_time_until_expiration > global_params.block_interval,
                       "Maximum transaction expiration time must be greater than a block interval" );
      }
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type committee_proposal_create_evaluator::do_apply( const committee_proposal_create_operation& op )
{ try {
   database& d = db();
   const auto& gpo = d.get_global_properties();
   const auto& dpo = d.get_dynamic_global_properties();

   uint16_t new_for = 0;
   if( op.proposer_opinion.valid() && *op.proposer_opinion == opinion_for )
      new_for = GRAPHENE_100_PERCENT / gpo.active_committee_members.size();

   const auto& new_committee_proposal_object = d.create<committee_proposal_object>( [&]( committee_proposal_object& cpo ){
      cpo.proposal_number          = dpo.next_committee_proposal_number;
      cpo.proposer                 = op.proposer;
      cpo.items                    = op.items;
      cpo.voting_closing_block_num = op.voting_closing_block_num;
      cpo.execution_block_num      = op.execution_block_num;
      cpo.expiration_block_num     = op.expiration_block_num;
      if( op.proposer_opinion.valid() )
         cpo.opinions.insert( std::make_pair( op.proposer, *op.proposer_opinion ) );
      cpo.approve_threshold        = cpo.get_approve_threshold();
      cpo.is_approved              = ( new_for >= cpo.approve_threshold ? true : false );
   });

   d.modify( dpo, [&]( dynamic_global_property_object& _dpo )
   {
      _dpo.next_committee_proposal_number += 1;
   } );

   const auto new_id = new_committee_proposal_object.id;

   if( new_committee_proposal_object.is_approved && d.head_block_num() >= op.execution_block_num )
      d.execute_committee_proposal( new_committee_proposal_object );

   return new_id;
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result committee_proposal_update_evaluator::do_evaluate( const committee_proposal_update_operation& op )
{ try {
   database& d = db();

   const auto& gpo = d.get_global_properties();
   const auto& current_committee = gpo.active_committee_members;

   FC_ASSERT( current_committee.find( op.account ) != current_committee.end(),
              "Account ${a} is not an active committee member",
              ("a",op.account) );


   proposal_obj = &d.get_committee_proposal_by_number( op.proposal_number );

   FC_ASSERT( d.head_block_num() <= proposal_obj->voting_closing_block_num,
              "Voting for proposal ${n} has closed, can not vote",
              ("n",op.proposal_number) );

   auto itr = proposal_obj->opinions.find( op.account );
   if( itr != proposal_obj->opinions.end() )
   {
      const auto old_opinion = itr->second;
      FC_ASSERT( old_opinion != op.opinion, "Opinion on proposal ${n} did not change.", ("n",op.proposal_number) );
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result committee_proposal_update_evaluator::do_apply( const committee_proposal_update_operation& op )
{ try {
   database& d = db();
   const auto& gpo = d.get_global_properties();

   d.modify( *proposal_obj, [&]( committee_proposal_object& cpo ){
      cpo.opinions[ op.account ] = op.opinion;
      uint8_t new_yeses = 0;
      for( const auto& opinion : cpo.opinions )
      {
         if( opinion.second == opinion_for )
            new_yeses += 1;
      }
      auto new_yes_percent = uint32_t( new_yeses ) * GRAPHENE_100_PERCENT / gpo.active_committee_members.size();
      bool new_approved = ( new_yes_percent >= cpo.approve_threshold );
      if( new_approved != cpo.is_approved )
         cpo.is_approved = new_approved;
   });

   if( proposal_obj->is_approved && d.head_block_num() >= proposal_obj->execution_block_num )
      d.execute_committee_proposal( *proposal_obj );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

} } // graphene::chain
