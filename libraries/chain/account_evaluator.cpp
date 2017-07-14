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

#include <fc/smart_ref_impl.hpp>

#include <graphene/chain/account_evaluator.hpp>
#include <graphene/chain/buyback.hpp>
#include <graphene/chain/buyback_object.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/internal_exceptions.hpp>
#include <graphene/chain/special_authority.hpp>
#include <graphene/chain/special_authority_object.hpp>
#include <graphene/chain/worker_object.hpp>

#include <algorithm>

namespace graphene { namespace chain {

void verify_authority_accounts( const database& db, const authority& a )
{
   const auto& chain_params = db.get_global_properties().parameters;
   GRAPHENE_ASSERT(
      a.num_auths() <= chain_params.maximum_authority_membership,
      internal_verify_auth_max_auth_exceeded,
      "Maximum authority membership exceeded" );
   for( const auto& acnt : a.account_auths )
   {
      GRAPHENE_ASSERT( db.find_object( acnt.first ) != nullptr,
         internal_verify_auth_account_not_found,
         "Account ${a} specified in authority does not exist",
         ("a", acnt.first) );
   }
   for( const auto& uid_auth : a.account_uid_auths )
   {
      GRAPHENE_ASSERT( db.find_account_id_by_uid( uid_auth.first.uid ).valid(),
         internal_verify_auth_account_not_found,
         "Account uid ${a} specified in authority does not exist",
         ("a", uid_auth.first.uid) );
   }
}

void verify_account_votes( const database& db, const account_options& options )
{
   // ensure account's votes satisfy requirements
   // NB only the part of vote checking that requires chain state is here,
   // the rest occurs in account_options::validate()

   // TODO review
   /*
   const auto& gpo = db.get_global_properties();
   const auto& chain_params = gpo.parameters;

   FC_ASSERT( options.num_witness <= chain_params.maximum_witness_count,
              "Voted for more witnesses than currently allowed (${c})", ("c", chain_params.maximum_witness_count) );
   FC_ASSERT( options.num_committee <= chain_params.maximum_committee_count,
              "Voted for more committee members than currently allowed (${c})", ("c", chain_params.maximum_committee_count) );

   uint32_t max_vote_id = gpo.next_available_vote_id;
   bool has_worker_votes = false;
   for( auto id : options.votes )
   {
      FC_ASSERT( id < max_vote_id );
      has_worker_votes |= (id.type() == vote_id_type::worker);
   }

   if( has_worker_votes && (db.head_block_time() >= HARDFORK_607_TIME) )
   {
      const auto& against_worker_idx = db.get_index_type<worker_index>().indices().get<by_vote_against>();
      for( auto id : options.votes )
      {
         if( id.type() == vote_id_type::worker )
         {
            FC_ASSERT( against_worker_idx.find( id ) == against_worker_idx.end() );
         }
      }
   }
   */

}


void_result account_create_evaluator::do_evaluate( const account_create_operation& op )
{ try {
   database& d = db();

   FC_ASSERT( d.find_account_id_by_uid(op.options.voting_account).valid(), "Invalid proxy account specified." );
   FC_ASSERT( fee_paying_account->is_registrar, "Only registrars may register an account." );
   const auto& referrer = d.get_account_by_uid( op.reg_info.referrer );
   FC_ASSERT( referrer.is_full_member, "The referrer must be a full member." );

   //TODO: check the parameters in reg_info against global parameters

   try
   {
      verify_authority_accounts( d, op.owner );
      verify_authority_accounts( d, op.active );
      verify_authority_accounts( d, op.secondary );
   }
   GRAPHENE_RECODE_EXC( internal_verify_auth_max_auth_exceeded, account_create_max_auth_exceeded )
   GRAPHENE_RECODE_EXC( internal_verify_auth_account_not_found, account_create_auth_account_not_found )

   //verify_account_votes( d, op.options );

   auto& acnt_indx = d.get_index_type<account_index>();
   {
      auto current_account_itr = acnt_indx.indices().get<by_uid>().find( op.uid );
      FC_ASSERT( current_account_itr == acnt_indx.indices().get<by_uid>().end(), "account uid already exists." );
   }
   {
      auto current_account_itr = acnt_indx.indices().get<by_name>().find( op.name );
      FC_ASSERT( current_account_itr == acnt_indx.indices().get<by_name>().end(), "account name already exists." );
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type account_create_evaluator::do_apply( const account_create_operation& o )
{ try {

   //TODO review
   database& d = db();

   const auto& new_acnt_object = d.create<account_object>( [&]( account_object& obj ){
         obj.uid              = o.uid;
         obj.name             = o.name;
         obj.owner            = o.owner;
         obj.active           = o.active;
         obj.secondary        = o.secondary;
         obj.options          = o.options;
         obj.reg_info         = o.reg_info;
         //TODO review: remove after related feature implemented
         // start
         if( o.name == "init0" || o.name == "init1" )
         {
            obj.is_registrar = true;
            obj.is_full_member = true;
         }
         if( o.name == "init3" || o.name == "init4" )
            obj.is_full_member = true;
         // end

         obj.create_time      = d.head_block_time();
         obj.last_update_time = d.head_block_time();

         obj.statistics = d.create<account_statistics_object>([&](account_statistics_object& s){s.owner = obj.uid;}).id;
   });

   return new_acnt_object.id;
} FC_CAPTURE_AND_RETHROW((o)) }


void_result account_manage_evaluator::do_evaluate( const account_manage_operation& o )
{ try {
   database& d = db();

   acnt = &d.get_account_by_uid( o.account );

   FC_ASSERT( acnt->reg_info.registrar == o.executor, "account should be managed by registrar" );

   const auto& ao = o.options.value;
   if( ao.can_post.valid() )
      FC_ASSERT( acnt->can_post != *ao.can_post, "can_post specified but didn't change" );
   if( ao.can_reply.valid() )
      FC_ASSERT( acnt->can_reply != *ao.can_reply, "can_reply specified but didn't change" );
   if( ao.can_rate.valid() )
      FC_ASSERT( acnt->can_rate != *ao.can_rate, "can_rate specified but didn't change" );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result account_manage_evaluator::do_apply( const account_manage_operation& o )
{ try {
   database& d = db();

   const auto& ao = o.options.value;
   d.modify( *acnt, [&](account_object& a){
      if( ao.can_post.valid() )
         a.can_post = *ao.can_post;
      if( ao.can_reply.valid() )
         a.can_reply = *ao.can_reply;
      if( ao.can_rate.valid() )
         a.can_rate = *ao.can_rate;
      a.last_update_time = d.head_block_time();
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result account_update_key_evaluator::do_evaluate( const account_update_key_operation& o )
{ try {
   database& d = db();

   acnt = &d.get_account_by_uid( o.uid );

   if( o.update_active )
   {
      auto& ka = acnt->active.key_auths;
      auto itr = ka.find( o.new_key );
      bool new_key_found = ( itr != ka.end() );
      FC_ASSERT( !new_key_found , "new_key is already in active authority" );

      itr_active = ka.find( o.old_key );
      bool old_key_found = ( itr_active != ka.end() );
      FC_ASSERT( old_key_found , "old_key is not in active authority" );
      active_weight = itr_active->second;
   }

   if( o.update_secondary )
   {
      auto& ka = acnt->secondary.key_auths;
      auto itr = ka.find( o.new_key );
      bool new_key_found = ( itr != ka.end() );
      FC_ASSERT( !new_key_found , "new_key is already in secondary authority" );

      itr_secondary = ka.find( o.old_key );
      bool old_key_found = ( itr_secondary != ka.end() );
      FC_ASSERT( old_key_found , "old_key is not in secondary authority" );
      secondary_weight = itr_secondary->second;
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result account_update_key_evaluator::do_apply( const account_update_key_operation& o )
{ try {
   database& d = db();
   d.modify( *acnt, [&](account_object& a){
      if( o.update_active )
      {
         a.active.key_auths.erase( itr_active );
         a.active.key_auths[ o.new_key ] = active_weight;
      }
      if( o.update_secondary )
      {
         a.secondary.key_auths.erase( itr_secondary );
         a.secondary.key_auths[ o.new_key ] = secondary_weight;
      }
      a.last_update_time = d.head_block_time();
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result account_update_auth_evaluator::do_evaluate( const account_update_auth_operation& o )
{ try {
   database& d = db();

   try
   {
      if( o.owner )  verify_authority_accounts( d, *o.owner );
      if( o.active ) verify_authority_accounts( d, *o.active );
      if( o.secondary ) verify_authority_accounts( d, *o.secondary );
   }
   GRAPHENE_RECODE_EXC( internal_verify_auth_max_auth_exceeded, account_update_max_auth_exceeded )
   GRAPHENE_RECODE_EXC( internal_verify_auth_account_not_found, account_update_auth_account_not_found )

   acnt = &d.get_account_by_uid( o.uid );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result account_update_auth_evaluator::do_apply( const account_update_auth_operation& o )
{ try {
   database& d = db();
   d.modify( *acnt, [&](account_object& a){
      if( o.owner )
         a.owner = *o.owner;
      if( o.active )
         a.active = *o.active;
      if( o.secondary )
         a.secondary = *o.secondary;
      if( o.memo_key )
         a.options.memo_key = *o.memo_key;
      a.last_update_time = d.head_block_time();
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result account_update_evaluator::do_evaluate( const account_update_operation& o )
{ try {
   database& d = db();

   try
   {
      if( o.owner )  verify_authority_accounts( d, *o.owner );
      if( o.active ) verify_authority_accounts( d, *o.active );
   }
   GRAPHENE_RECODE_EXC( internal_verify_auth_max_auth_exceeded, account_update_max_auth_exceeded )
   GRAPHENE_RECODE_EXC( internal_verify_auth_account_not_found, account_update_auth_account_not_found )

   acnt = &o.account(d);

   if( o.new_options.valid() )
      verify_account_votes( d, *o.new_options );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result account_update_evaluator::do_apply( const account_update_operation& o )
{ try {
   database& d = db();
   d.modify( *acnt, [&](account_object& a){
      if( o.owner )
      {
         a.owner = *o.owner;
         a.top_n_control_flags = 0;
      }
      if( o.active )
      {
         a.active = *o.active;
         a.top_n_control_flags = 0;
      }
      if( o.new_options ) a.options = *o.new_options;
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result account_whitelist_evaluator::do_evaluate(const account_whitelist_operation& o)
{ try {
   database& d = db();

   listed_account = &o.account_to_list(d);
   if( !d.get_global_properties().parameters.allow_non_member_whitelists )
      FC_ASSERT(o.authorizing_account(d).is_lifetime_member());

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result account_whitelist_evaluator::do_apply(const account_whitelist_operation& o)
{ try {
   database& d = db();

   d.modify(*listed_account, [&o](account_object& a) {
      if( o.new_listing & o.white_listed )
         a.whitelisting_accounts.insert(o.authorizing_account);
      else
         a.whitelisting_accounts.erase(o.authorizing_account);

      if( o.new_listing & o.black_listed )
         a.blacklisting_accounts.insert(o.authorizing_account);
      else
         a.blacklisting_accounts.erase(o.authorizing_account);
   });

   /** for tracking purposes only, this state is not needed to evaluate */
   d.modify( o.authorizing_account(d), [&]( account_object& a ) {
     if( o.new_listing & o.white_listed )
        a.whitelisted_accounts.insert( o.account_to_list );
     else
        a.whitelisted_accounts.erase( o.account_to_list );

     if( o.new_listing & o.black_listed )
        a.blacklisted_accounts.insert( o.account_to_list );
     else
        a.blacklisted_accounts.erase( o.account_to_list );
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result account_upgrade_evaluator::do_evaluate(const account_upgrade_evaluator::operation_type& o)
{ try {
   database& d = db();

   account = &d.get(o.account_to_upgrade);
   FC_ASSERT(!account->is_lifetime_member());

   return {};
//} FC_CAPTURE_AND_RETHROW( (o) ) }
} FC_RETHROW_EXCEPTIONS( error, "Unable to upgrade account '${a}'", ("a",o.account_to_upgrade(db()).name) ) }

void_result account_upgrade_evaluator::do_apply(const account_upgrade_evaluator::operation_type& o)
{ try {
   database& d = db();

   d.modify(*account, [&](account_object& a) {
      if( o.upgrade_to_lifetime_member )
      {
         // Upgrade to lifetime member. I don't care what the account was before.
         a.statistics(d).process_fees(a, d);
         a.membership_expiration_date = time_point_sec::maximum();
         a.referrer = a.registrar = a.lifetime_referrer = a.get_id();
         a.lifetime_referrer_fee_percentage = GRAPHENE_100_PERCENT - a.network_fee_percentage;
      } else if( a.is_annual_member(d.head_block_time()) ) {
         // Renew an annual subscription that's still in effect.
         FC_ASSERT( d.head_block_time() <= HARDFORK_613_TIME );
         FC_ASSERT(a.membership_expiration_date - d.head_block_time() < fc::days(3650),
                   "May not extend annual membership more than a decade into the future.");
         a.membership_expiration_date += fc::days(365);
      } else {
         // Upgrade from basic account.
         FC_ASSERT( d.head_block_time() <= HARDFORK_613_TIME );
         a.statistics(d).process_fees(a, d);
         assert(a.is_basic_account(d.head_block_time()));
         a.referrer = a.get_id();
         a.membership_expiration_date = d.head_block_time() + fc::days(365);
      }
   });

   return {};
} FC_RETHROW_EXCEPTIONS( error, "Unable to upgrade account '${a}'", ("a",o.account_to_upgrade(db()).name) ) }

} } // graphene::chain
