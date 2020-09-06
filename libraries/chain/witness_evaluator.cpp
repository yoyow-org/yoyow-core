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
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/hardfork.hpp>

namespace graphene { namespace chain {

void_result witness_create_evaluator::do_evaluate( const witness_create_operation& op )
{ try {
   database& d = db();
   //FC_ASSERT(db().get(op.account).is_lifetime_member());
   account_stats = &d.get_account_statistics_by_uid( op.account );
   account_obj = &d.get_account_by_uid( op.account );

   const auto& global_params = d.get_global_properties().parameters;
   // don't enforce pledge check for initial witnesses
   if( d.head_block_num() > 0 )
      FC_ASSERT( op.pledge.amount >= global_params.min_witness_pledge,
                 "Insufficient pledge: provided ${p}, need ${r}",
                 ("p",d.to_pretty_string(op.pledge))
                 ("r",d.to_pretty_core_string(global_params.min_witness_pledge)) );

   // releasing witness pledge can be reused.
   auto available_balance = account_stats->get_available_core_balance(pledge_balance_type::Witness, d);
   FC_ASSERT( available_balance >= op.pledge.amount,
              "Insufficient Balance: account ${a}'s available balance of ${b} is less than required ${r}",
              ("a",op.account)
              ("b",d.to_pretty_core_string(available_balance))
              ("r",d.to_pretty_string(op.pledge)) );

   const witness_object* maybe_found = d.find_witness_by_uid( op.account );
   FC_ASSERT( maybe_found == nullptr, "This account is already a witness" );

   const dynamic_global_property_object& dpo = d.get_dynamic_global_properties();
   if (dpo.enabled_hardfork_version >= ENABLE_HEAD_FORK_05 && op.extensions.valid() && op.extensions->value.bonus_rate.valid())
   {
      auto bonus_rate = *(op.extensions->value.bonus_rate);
      auto max_pledge_mining_bonus_rate = global_params.get_extension_params().max_pledge_mining_bonus_rate;
      FC_ASSERT(bonus_rate >= 0 && bonus_rate <= max_pledge_mining_bonus_rate,
         "bonus rate should be between 0 and ${max_rate}", ("max_rate", max_pledge_mining_bonus_rate));
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type witness_create_evaluator::do_apply( const witness_create_operation& op )
{ try {
   database& d = db();

   const auto& global_params = d.get_global_properties().parameters;

   const dynamic_global_property_object& dpo = d.get_dynamic_global_properties();
   const auto& new_witness_object = d.create<witness_object>( [&]( witness_object& wit ){
      wit.account             = op.account;
      wit.name                = account_obj->name;
      wit.sequence            = account_stats->last_witness_sequence + 1;
      //wit.is_valid            = true; // default
      wit.signing_key         = op.block_signing_key;
      wit.pledge              = op.pledge.amount.value;
      wit.pledge_last_update  = d.head_block_time();

      wit.average_pledge_last_update       = d.head_block_time();
      if( wit.pledge > 0 )
         wit.average_pledge_next_update_block = d.head_block_num() + global_params.witness_avg_pledge_update_interval;
      else // init witnesses
         wit.average_pledge_next_update_block = -1;

      wit.url                 = op.url;

      if (dpo.enabled_hardfork_version >= ENABLE_HEAD_FORK_05 && op.extensions.valid())
      {
         auto witness_ext = op.extensions->value;
         if (witness_ext.can_pledge.valid())
            wit.can_pledge = *witness_ext.can_pledge;
         if (witness_ext.bonus_rate.valid())
            wit.bonus_rate = *witness_ext.bonus_rate;
      }
   });

   const uint64_t csaf_window = global_params.csaf_accumulate_window;
   auto block_time = d.head_block_time();
   d.modify(*account_stats, [&](_account_statistics_object& s) {
      s.last_witness_sequence += 1;
      auto releasing_pledge = s.get_releasing_pledge(GRAPHENE_CORE_ASSET_AID, pledge_balance_type::Witness, d);
      if (releasing_pledge <= op.pledge.amount && dpo.enabled_hardfork_version == ENABLE_HEAD_FORK_04)
         s.update_coin_seconds_earned(csaf_window, block_time, d, ENABLE_HEAD_FORK_04);
   });

   if (account_stats->pledge_balance_ids.count(pledge_balance_type::Witness))
   {
      const auto& wpb_obj = d.get(account_stats->pledge_balance_ids.at(pledge_balance_type::Witness));
      d.modify(wpb_obj, [&](pledge_balance_object& obj) {
         obj.update_pledge(op.pledge, -1,d);//In this case, the second parameter(-1) is invalid
      });
   }
   else {//create pledge_balance_obj
      const auto& new_pledge_balance_obj = d.create<pledge_balance_object>([&](pledge_balance_object& obj){
         obj.superior_index = op.account;
         obj.type = pledge_balance_type::Witness;
         obj.asset_id = GRAPHENE_CORE_ASSET_AID;
         obj.pledge = op.pledge.amount;
      });
      d.modify(*account_stats, [&](_account_statistics_object& s) {
         s.pledge_balance_ids.emplace(pledge_balance_type::Witness, new_pledge_balance_obj.id);
      });
   }

   d.modify(dpo, [&](dynamic_global_property_object& _dpo) {
      _dpo.total_witness_pledge += op.pledge.amount;
   });

   return new_witness_object.id;
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result witness_update_evaluator::do_evaluate( const witness_update_operation& op )
{ try {
   database& d = db();
   account_stats = &d.get_account_statistics_by_uid( op.account );
   witness_obj   = &d.get_witness_by_uid( op.account );

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

         // releasing witness pledge can be reused.
         auto available_balance = account_stats->get_available_core_balance(pledge_balance_type::Witness, d);
         FC_ASSERT( available_balance >= op.new_pledge->amount,
                    "Insufficient Balance: account ${a}'s available balance of ${b} is less than required ${r}",
                    ("a",op.account)
                    ("b",d.to_pretty_core_string(available_balance))
                    ("r",d.to_pretty_string(*op.new_pledge)) );
      }
      else if( op.new_pledge->amount == 0 ) // resign
      {
         const auto& active_witnesses = d.get_global_properties().active_witnesses;
         FC_ASSERT( active_witnesses.find( op.account ) == active_witnesses.end(),
                    "Active witness can not resign" );
      }
   }
   else // when updating witness, always check pledge
   {
      FC_ASSERT( witness_obj->pledge >= global_params.min_witness_pledge,
                 "Insufficient pledge: has ${p}, need ${r}",
                 ("p",d.to_pretty_core_string(witness_obj->pledge))
                 ("r",d.to_pretty_core_string(global_params.min_witness_pledge)) );
   }

   if( op.new_url.valid() )
      FC_ASSERT( *op.new_url != witness_obj->url, "new_url specified but did not change" );

   const dynamic_global_property_object& dpo = d.get_dynamic_global_properties();
   if (dpo.enabled_hardfork_version >= ENABLE_HEAD_FORK_05 && op.extensions.valid())
   {
      auto witness_ext = op.extensions->value;
      if (witness_ext.can_pledge.valid()) 
      {
         FC_ASSERT(*witness_ext.can_pledge != witness_obj->can_pledge, 
            "can_pledge specified but did not change");
      }
      if (witness_ext.bonus_rate.valid()) 
      {
         FC_ASSERT(*witness_ext.bonus_rate != witness_obj->bonus_rate, 
            "bonus_rate specified but did not change");
         auto max_pledge_mining_bonus_rate = global_params.get_extension_params().max_pledge_mining_bonus_rate;
         FC_ASSERT(*witness_ext.bonus_rate >= 0 && *witness_ext.bonus_rate <= max_pledge_mining_bonus_rate,
            "bonus rate should be between 0 and ${max_rate}", ("max_rate", max_pledge_mining_bonus_rate));
      }      
   }

   //check pledge balance object 
   if (account_stats->pledge_balance_ids.count(pledge_balance_type::Witness) == 0)
      FC_ASSERT(false, "pledge balance object is nonexistent");

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result witness_update_evaluator::do_apply( const witness_update_operation& op )
{ try {
   database& d = db();

   const auto& global_params = d.get_global_properties().parameters;
   const dynamic_global_property_object& dpo = d.get_dynamic_global_properties();

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
      const auto& wpb_obj = d.get(account_stats->pledge_balance_ids.at(pledge_balance_type::Witness));
      d.modify(wpb_obj, [&](pledge_balance_object& obj) {
         obj.update_pledge(*(op.new_pledge), d.head_block_num() + global_params.witness_pledge_release_delay,d);
      });

      d.modify( *witness_obj, [&]( witness_object& wit ) {
         wit.is_valid = false; // will be processed later
         wit.average_pledge_next_update_block = -1;
         wit.by_pledge_scheduled_time = UINT128_MAX_VALUE;
         wit.by_vote_scheduled_time = UINT128_MAX_VALUE;
      });
      share_type delta = op.new_pledge->amount - witness_obj->pledge;
      if (dpo.enabled_hardfork_version >= ENABLE_HEAD_FORK_05){
         d.modify(dpo, [&](dynamic_global_property_object& _dpo) {
            _dpo.total_witness_pledge += (delta - witness_obj->total_mining_pledge);
         });
      }
      else{
         d.modify(dpo, [&](dynamic_global_property_object& _dpo) {
            _dpo.resign_witness_pledge_before_05 += delta;
         });
      }
   }
   else // change pledge
   {
      // update account stats
      share_type delta = op.new_pledge->amount - witness_obj->pledge;
      if( delta > 0 ) // more pledge
      {
         const uint64_t csaf_window = global_params.csaf_accumulate_window;
         auto block_time = d.head_block_time();
         d.modify(*account_stats, [&](_account_statistics_object& s) {
            auto releasing_pledge = s.get_releasing_pledge(GRAPHENE_CORE_ASSET_AID, pledge_balance_type::Witness, d);
            if (releasing_pledge <= delta && dpo.enabled_hardfork_version == ENABLE_HEAD_FORK_04)
               s.update_coin_seconds_earned(csaf_window, block_time, d, ENABLE_HEAD_FORK_04);
         });
      }

      const auto& wpb_obj = d.get(account_stats->pledge_balance_ids.at(pledge_balance_type::Witness));
      d.modify(wpb_obj, [&](pledge_balance_object& obj) {
         obj.update_pledge(*(op.new_pledge), d.head_block_num() + global_params.witness_pledge_release_delay,d);
      });

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

      d.modify(dpo, [&](dynamic_global_property_object& _dpo) {
         _dpo.total_witness_pledge += delta;
      });

      // update schedule
      d.update_witness_avg_pledge( *witness_obj );
   }

   if (dpo.enabled_hardfork_version >= ENABLE_HEAD_FORK_05 && op.extensions.valid())
   {
      auto witness_ext = op.extensions->value;
      d.modify(*witness_obj, [&](witness_object& wit) {       
         if (witness_ext.can_pledge.valid())
            wit.can_pledge = *witness_ext.can_pledge;
         if (witness_ext.bonus_rate.valid())
            wit.bonus_rate = *witness_ext.bonus_rate;
      }); 

      if (witness_ext.can_pledge.valid() && !*(witness_ext.can_pledge)) {
         d.update_pledge_mining_bonus_by_witness(*witness_obj);
         d.resign_pledge_mining(*witness_obj);

         d.modify(dpo, [&](dynamic_global_property_object& _dpo) {
            _dpo.total_witness_pledge -= witness_obj->total_mining_pledge;
         });
         d.modify(*witness_obj, [&](witness_object& wit) {
            wit.total_mining_pledge = 0;

            wit.unhandled_bonus = 0;
            wit.need_distribute_bonus = 0;
            wit.already_distribute_bonus = 0;
            wit.last_update_bonus_block_num = d.head_block_num();
            wit.bonus_per_pledge.clear();
         });
      }
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result witness_vote_update_evaluator::do_evaluate( const witness_vote_update_operation& op )
{ try {
   database& d = db();
   account_stats = &d.get_account_statistics_by_uid( op.voter );

   FC_ASSERT( account_stats->can_vote == true, "This account can not vote" );

   const auto& global_params = d.get_global_properties().parameters;
   FC_ASSERT( account_stats->core_balance >= global_params.min_governance_voting_balance,
              "Need more balance to be able to vote: have ${b}, need ${r}",
              ("b",d.to_pretty_core_string(account_stats->core_balance))
              ("r",d.to_pretty_core_string(global_params.min_governance_voting_balance)) );

   const auto max_witnesses = global_params.max_witnesses_voted_per_account;
   FC_ASSERT( op.witnesses_to_add.size() <= max_witnesses,
              "Trying to vote for ${n} witnesses, more than allowed maximum: ${m}",
              ("n",op.witnesses_to_add.size())("m",max_witnesses) );

   for( const auto uid : op.witnesses_to_remove )
   {
      const witness_object& wit = d.get_witness_by_uid( uid );
      witnesses_to_remove.push_back( &wit );
   }
   for( const auto uid : op.witnesses_to_add )
   {
      const witness_object& wit = d.get_witness_by_uid( uid );
      witnesses_to_add.push_back( &wit );
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
      FC_ASSERT( op.witnesses_to_remove.empty(), "Not voting for any witness, or votes were no longer valid, can not remove" );
   else if( voter_obj->proxy_uid != GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID ) // voting with a proxy
   {
      // check if the proxy is still valid
      const voter_object* current_proxy_voter_obj = d.find_voter( voter_obj->proxy_uid, voter_obj->proxy_sequence );
      FC_ASSERT( current_proxy_voter_obj != nullptr, "proxy voter should exist" );
      bool current_proxy_still_valid = d.check_voter_valid( *current_proxy_voter_obj, true );
      if( current_proxy_still_valid ) // still valid
         FC_ASSERT( op.witnesses_to_remove.empty() && op.witnesses_to_add.empty(),
                    "Now voting with a proxy, can not add or remove witness" );
      else // no longer valid
      {
         invalid_current_proxy_voter_obj = current_proxy_voter_obj;
         FC_ASSERT( op.witnesses_to_remove.empty(),
                    "Was voting with a proxy but it is now invalid, so not voting for any witness, can not remove" );
      }
   }
   else // voting by self
   {
      // check for voted witnesses whom have became invalid
      uint16_t witnesses_voted = voter_obj->number_of_witnesses_voted;
      const auto& idx = d.get_index_type<witness_vote_index>().indices().get<by_voter_seq>();
      auto itr = idx.lower_bound( std::make_tuple( op.voter, voter_obj->sequence ) );
      while( itr != idx.end() && itr->voter_uid == op.voter && itr->voter_sequence == voter_obj->sequence )
      {
         const witness_object* witness = d.find_witness_by_uid( itr->witness_uid );
         if( witness == nullptr || witness->sequence != itr->witness_sequence )
         {
            invalid_witness_votes_to_remove.push_back( &(*itr) );
            --witnesses_voted;
         }
         ++itr;
      }

      FC_ASSERT( op.witnesses_to_remove.size() <= witnesses_voted,
                 "Trying to remove ${n} witnesses, more than voted: ${m}",
                 ("n",op.witnesses_to_remove.size())("m",witnesses_voted) );
      uint16_t new_total = witnesses_voted - op.witnesses_to_remove.size() + op.witnesses_to_add.size();
      FC_ASSERT( new_total <= max_witnesses,
                 "Trying to vote for ${n} witnesses, more than allowed maximum: ${m}",
                 ("n",new_total)("m",max_witnesses) );

      for( const auto& wit : witnesses_to_remove )
      {
         const witness_vote_object* wit_vote = d.find_witness_vote( op.voter, voter_obj->sequence, wit->account, wit->sequence );
         FC_ASSERT( wit_vote != nullptr, "Not voting for witness ${w}, can not remove", ("w",wit->account) );
         witness_votes_to_remove.push_back( wit_vote );
      }
      for( const auto& wit : witnesses_to_add )
      {
         const witness_vote_object* wit_vote = d.find_witness_vote( op.voter, voter_obj->sequence, wit->account, wit->sequence );
         FC_ASSERT( wit_vote == nullptr, "Already voting for witness ${w}, can not add", ("w",wit->account) );
      }
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result witness_vote_update_evaluator::do_apply( const witness_vote_update_operation& op )
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

      // remove invalid witness votes
      for( const auto& wit_vote : invalid_witness_votes_to_remove )
         d.remove( *wit_vote );

      // remove requested witness votes
      total_votes = voter_obj->total_votes();
      const size_t total_remove = witnesses_to_remove.size();
      for( uint16_t i = 0; i < total_remove; ++i )
      {
         d.adjust_witness_votes( *witnesses_to_remove[i], -total_votes );
         d.remove( *witness_votes_to_remove[i] );
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
         v.number_of_witnesses_voted = v.number_of_witnesses_voted
                                     - invalid_witness_votes_to_remove.size()
                                     - witnesses_to_remove.size()
                                     + witnesses_to_add.size();
      });
   }
   else // need to create a new voter object for this account
   {
      d.modify(*account_stats, [&](_account_statistics_object& s) {
         s.is_voter = true;
         s.last_voter_sequence += 1;
      });

      voter_obj = &d.create<voter_object>( [&]( voter_object& v ){
         v.uid               = op.voter;
         v.sequence          = account_stats->last_voter_sequence;
         //v.is_valid          = true; // default
         v.votes             = account_stats->get_votes_from_core_balance();
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

         v.number_of_witnesses_voted         = witnesses_to_add.size();
      });
   }

   // add requested witness votes
   const size_t total_add = witnesses_to_add.size();
   for( uint16_t i = 0; i < total_add; ++i )
   {
      d.create<witness_vote_object>( [&]( witness_vote_object& o ){
         o.voter_uid        = op.voter;
         o.voter_sequence   = voter_obj->sequence;
         o.witness_uid      = witnesses_to_add[i]->account;
         o.witness_sequence = witnesses_to_add[i]->sequence;
      });
      if( total_votes > 0 )
         d.adjust_witness_votes( *witnesses_to_add[i], total_votes );
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result witness_collect_pay_evaluator::do_evaluate( const witness_collect_pay_operation& op )
{ try {
   database& d = db();
   account_stats = &d.get_account_statistics_by_uid( op.account );

   FC_ASSERT( account_stats->uncollected_witness_pay >= op.pay.amount,
              "Can not collect so much: have ${b}, requested ${r}",
              ("b",d.to_pretty_core_string(account_stats->uncollected_witness_pay))
              ("r",d.to_pretty_string(op.pay)) );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result witness_collect_pay_evaluator::do_apply( const witness_collect_pay_operation& op )
{ try {
   database& d = db();

   d.adjust_balance( op.account, op.pay );
   d.modify(*account_stats, [&](_account_statistics_object& s) {
      s.uncollected_witness_pay -= op.pay.amount;
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result witness_report_evaluator::do_evaluate( const witness_report_operation& op )
{ try {
   database& d = db();
   d.get_account_by_uid( op.reporter ); // check if the reporter account exists
   account_stats = &d.get_account_statistics_by_uid( op.first_block.witness );

   const auto head_block_time = d.head_block_time();
   const auto reporting_block_time = op.first_block.timestamp;
   FC_ASSERT( reporting_block_time <= head_block_time, "Can not report a block which has a timestamp in the future" );

   const auto head_block_num = d.head_block_num();
   reporting_block_num = op.first_block.block_num();
   FC_ASSERT( reporting_block_num <= head_block_num,
              "Can not report a block that has a block number larger than current head block number" );

   const auto& global_params = d.get_global_properties().parameters;
   FC_ASSERT( reporting_block_num + global_params.witness_report_prosecution_period >= head_block_num,
              "Can not report a block that is more than ${n} blocks old",
              ("n", global_params.witness_report_prosecution_period) );

   if( !global_params.witness_report_allow_pre_last_block )
   {
      FC_ASSERT( reporting_block_num >= account_stats->witness_last_confirmed_block_num,
                 "Can not report a block that is older than this witness's last confirmed block: #${n}",
                 ("n", account_stats->witness_last_confirmed_block_num) );
   }

   FC_ASSERT( reporting_block_num > account_stats->witness_last_reported_block_num,
              "Can only report blocks newer than this witness's last reported block: #${n}",
              ("n", account_stats->witness_last_reported_block_num) );

   block_id_type block_id_on_chain = d.fetch_block_id_for_num( reporting_block_num );
   FC_ASSERT( block_id_on_chain == op.first_block.id() || block_id_on_chain == op.second_block.id(),
              "Either first block or second block should be on current chain" );

   //check pledge balance object 
   if (account_stats->pledge_balance_ids.count(pledge_balance_type::Witness) == 0)
      FC_ASSERT(false, "pledge balance object is nonexistent");

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result witness_report_evaluator::do_apply( const witness_report_operation& op )
{ try {
   database& d = db();
   const auto& global_params = d.get_global_properties().parameters;

   // deactivate witness
   const witness_object* witness_obj = d.find_witness_by_uid( op.first_block.witness );
   if( witness_obj != nullptr )
   {
      d.modify( *witness_obj, [&]( witness_object& w ) {
         w.signing_key = public_key_type();
      });
   }

   const auto& pledge_balance_obj = d.get(account_stats->pledge_balance_ids.at(pledge_balance_type::Witness));
   // check if need to deduct from pledge
   share_type total = std::min(global_params.witness_report_pledge_deduction_amount, pledge_balance_obj.total_unrelease_pledge());
   if( total > 0 )
   {
      // something to deduct
      share_type from_releasing = std::min(pledge_balance_obj.total_releasing_pledge, total);
      share_type from_pledge = total - from_releasing;
      // update account stats object
      const uint64_t csaf_window = d.get_global_properties().parameters.csaf_accumulate_window;
      auto block_time = d.head_block_time();
      const dynamic_global_property_object& dpo = d.get_dynamic_global_properties();
      d.modify(*account_stats, [&](_account_statistics_object& s) {
         if (dpo.enabled_hardfork_version == ENABLE_HEAD_FORK_04)
             s.update_coin_seconds_earned(csaf_window, block_time, d, ENABLE_HEAD_FORK_04);
         d.modify(pledge_balance_obj, [&](pledge_balance_object& _pbo) {
            if (from_releasing > 0)
            {
               _pbo.reduce_releasing(from_releasing);
            }
            _pbo.pledge -= from_pledge;
         });
         s.witness_last_reported_block_num = reporting_block_num;
         s.witness_total_reported += 1;
      });

      d.modify(dpo, [&](dynamic_global_property_object& _dpo) {
         _dpo.total_witness_pledge -= total;
      });
      
      // update witness object
      if( from_pledge > 0 && witness_obj != nullptr )
      {
         // update position
         d.update_witness_avg_pledge( *witness_obj );

         // update witness data
         d.modify( *witness_obj, [&]( witness_object& wit ) {
            wit.pledge              -= from_pledge.value;
            wit.pledge_last_update  =  d.head_block_time();
         });

         // update schedule
         d.update_witness_avg_pledge( *witness_obj );
      }
      // adjust balance, should be ok, and will take care of votes
      d.adjust_balance( op.first_block.witness, -total );
      // adjust total supply
      d.modify( d.get_core_asset().dynamic_data(d), [&]( asset_dynamic_data_object& o )
      {
         o.current_supply -= total;
      });
   }
   else
   {
      // nothing to deduct
      d.modify(*account_stats, [&](_account_statistics_object& s) {
         s.witness_last_reported_block_num = reporting_block_num;
         s.witness_total_reported += 1;
      });
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }


} } // graphene::chain
