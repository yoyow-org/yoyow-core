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
#include <graphene/chain/database.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/internal_exceptions.hpp>

#include <algorithm>

namespace graphene { namespace chain {

void verify_authority_accounts( const database& db, const authority& a )
{
   const auto& chain_params = db.get_global_properties().parameters;
   GRAPHENE_ASSERT(
      a.num_auths() <= chain_params.maximum_authority_membership,
      internal_verify_auth_max_auth_exceeded,
      "Maximum authority membership exceeded" );
   for( const auto& uid_auth : a.account_uid_auths )
   {
      GRAPHENE_ASSERT( db.find_account_id_by_uid( uid_auth.first.uid ).valid(),
         internal_verify_auth_account_not_found,
         "Account uid ${a} specified in authority does not exist",
         ("a", uid_auth.first.uid) );
   }
}

void_result account_create_evaluator::do_evaluate( const account_create_operation& op )
{ try {
   database& d = db();

   FC_ASSERT( fee_paying_account->is_registrar, "Only registrars may register an account." );
   const auto& referrer = d.get_account_by_uid( op.reg_info.referrer );
   FC_ASSERT( referrer.is_full_member, "The referrer must be a valid platform or full member." );

/*
   if( d.head_block_num() > 0 )
   {
      const auto& platform = d.get_platform_by_owner( op.reg_info.referrer );
      FC_ASSERT( platform.is_valid, "The referrer must be a valid platform." );
   }
*/
   //TODO: check the parameters in reg_info against global parameters

   try
   {
      verify_authority_accounts( d, op.owner );
      verify_authority_accounts( d, op.active );
      verify_authority_accounts( d, op.secondary );
   }
   GRAPHENE_RECODE_EXC( internal_verify_auth_max_auth_exceeded, account_create_max_auth_exceeded )
   GRAPHENE_RECODE_EXC( internal_verify_auth_account_not_found, account_create_auth_account_not_found )

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
   const platform_object* platform = d.find_platform_by_owner(o.reg_info.registrar);

   const auto& new_acnt_object = d.create<account_object>( [&]( account_object& obj ){
         obj.uid                  = o.uid;
         obj.name                 = o.name;
         obj.owner                = o.owner;
         obj.active               = o.active;
         obj.secondary            = o.secondary;
         obj.memo_key             = o.memo_key;
         obj.reg_info             = o.reg_info;
         obj.register_by_platform = platform == nullptr ? false : true;
         obj.create_time          = d.head_block_time();
         obj.last_update_time     = d.head_block_time();

         obj.statistics = d.create<account_statistics_object>([&](account_statistics_object& s){s.owner = obj.uid;}).id;
   });

   return new_acnt_object.id;
} FC_CAPTURE_AND_RETHROW((o)) }


void_result account_manage_evaluator::do_evaluate( const account_manage_operation& o )
{ try {
   database& d = db();

   acnt = &d.get_account_by_uid( o.account );

   const account_object& registrar = d.get_account_by_uid( acnt->reg_info.registrar );
   if( registrar.is_registrar )
      FC_ASSERT( acnt->reg_info.registrar == o.executor, "account should be managed by registrar" );
   else
   {
      const account_uid_type takeover_registrar = d.get_registrar_takeover_object( registrar.uid ).takeover_registrar;
      FC_ASSERT( takeover_registrar == o.executor, "account should be managed by registrar" );
   }

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
   GRAPHENE_RECODE_EXC( internal_verify_auth_max_auth_exceeded, account_update_auth_max_auth_exceeded )
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
         a.memo_key = *o.memo_key;
      a.last_update_time = d.head_block_time();
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result account_auth_platform_evaluator::do_evaluate( const account_auth_platform_operation& o )
{ try {
   database& d = db();

   FC_ASSERT( d.head_block_time() >= HARDFORK_0_2_1_TIME, "Can only be account_auth_platform after HARDFORK_0_2_1_TIME" );

   if (o.extensions.valid() && (d.head_block_time() > HARDFORK_0_4_TIME))
       ext_para = &o.extensions->value;

   acnt = &d.get_account_by_uid( o.uid );

   auto& ka = acnt->secondary.account_uid_auths;
   auto itr = ka.find( authority::account_uid_auth_type( o.platform, authority::secondary_auth ) );
   found = ( itr != ka.end() );
   if (((!ext_para) && d.head_block_time() >= HARDFORK_0_4_TIME) || (d.head_block_time() < HARDFORK_0_4_TIME))
       FC_ASSERT( !found, "platform ${p} is already in secondary authority", ("p", o.platform) );

   authority auth = acnt->secondary;
   authority::account_uid_auth_type auat( o.platform, authority::secondary_auth );
   //auat.uid = o.platform;
   //auat.auth_type = authority::secondary_auth;
   auth.add_authority( auat, acnt->secondary.weight_threshold );

   try
   {
      verify_authority_accounts( d, auth );
   }
   GRAPHENE_RECODE_EXC( internal_verify_auth_max_auth_exceeded, account_update_auth_max_auth_exceeded )
   GRAPHENE_RECODE_EXC( internal_verify_auth_account_not_found, account_update_auth_account_not_found )

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result account_auth_platform_evaluator::do_apply( const account_auth_platform_operation& o )
{ try {
   database& d = db();
   if (((!ext_para) && d.head_block_time() >= HARDFORK_0_4_TIME) || (d.head_block_time() < HARDFORK_0_4_TIME))
   {
       authority::account_uid_auth_type auat(o.platform, authority::secondary_auth);
       d.modify(*acnt, [&](account_object& a){
           a.secondary.add_authority(auat, acnt->secondary.weight_threshold);
           a.last_update_time = d.head_block_time();
       });

       if (d.head_block_time() >= HARDFORK_0_4_TIME){
           d.create<account_auth_platform_object>([&](account_auth_platform_object& obj){
               obj.account = o.uid;
               obj.platform = o.platform;
               obj.max_limit = GRAPHENE_MAX_PLATFORM_LIMIT_PREPAID;
               obj.permission_flags = account_auth_platform_object::Platform_Permission_Forward |
                                      account_auth_platform_object::Platform_Permission_Liked |
                                      account_auth_platform_object::Platform_Permission_Buyout |
                                      account_auth_platform_object::Platform_Permission_Comment |
                                      account_auth_platform_object::Platform_Permission_Reward |
                                      account_auth_platform_object::Platform_Permission_Post;
           });
       }
   }
   else
   {
       if (!found)
       {
           authority::account_uid_auth_type auat(o.platform, authority::secondary_auth);
           d.modify(*acnt, [&](account_object& a){
               a.secondary.add_authority(auat, acnt->secondary.weight_threshold);
               a.last_update_time = d.head_block_time();
           });
       }

       auto get_platform_auth_object = [&](const account_uid_type account_uid, const account_uid_type platform_uid)
       {
           if (d.find_account_auth_platform_object_by_account_platform(account_uid, platform_uid))
           {
               return d.get_account_auth_platform_object_by_account_platform(account_uid, platform_uid);
           }
           else
           {
               return d.create<account_auth_platform_object>([&](account_auth_platform_object& obj){
                   obj.account = account_uid;
                   obj.platform = platform_uid;
                   obj.max_limit = GRAPHENE_MAX_PLATFORM_LIMIT_PREPAID;
                   obj.permission_flags = account_auth_platform_object::Platform_Permission_Forward |
                                          account_auth_platform_object::Platform_Permission_Liked |
                                          account_auth_platform_object::Platform_Permission_Buyout |
                                          account_auth_platform_object::Platform_Permission_Comment |
                                          account_auth_platform_object::Platform_Permission_Reward |
                                          account_auth_platform_object::Platform_Permission_Post;
               });
           }
       };

       d.modify(get_platform_auth_object(o.uid, o.platform), [&](account_auth_platform_object& a){
           if (ext_para->limit_for_platform.valid())
               a.max_limit = *(ext_para->limit_for_platform);
           if (ext_para->permission_flags.valid())
               a.permission_flags = *(ext_para->permission_flags);
           if (ext_para->memo.valid())
               a.memo = *(ext_para->memo);
       });
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result account_cancel_auth_platform_evaluator::do_evaluate( const account_cancel_auth_platform_operation& o )
{ try {
   database& d = db();

   FC_ASSERT( d.head_block_time() >= HARDFORK_0_2_1_TIME, "Can only be account_cancel_auth_platform after HARDFORK_0_2_1_TIME" );

   acnt = &d.get_account_by_uid( o.uid );
   auto& ka = acnt->secondary.account_uid_auths;
   auto itr = ka.find( authority::account_uid_auth_type( o.platform, authority::secondary_auth ) );
   bool found = ( itr != ka.end() );
   FC_ASSERT( found, "platform ${p} is not in secondary authority", ("p", o.platform) );
   
   if (d.head_block_time() >= HARDFORK_0_4_TIME)
       d.get_account_auth_platform_object_by_account_platform(o.uid, o.platform);

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result account_cancel_auth_platform_evaluator::do_apply( const account_cancel_auth_platform_operation& o )
{ try {
   database& d = db();
   auto& ka = acnt->secondary.account_uid_auths;
   auto itr = ka.find( authority::account_uid_auth_type( o.platform, authority::secondary_auth ) );
   d.modify( *acnt, [&](account_object& a){
      a.secondary.account_uid_auths.erase(itr);
      a.last_update_time = d.head_block_time();
   });

   if (d.head_block_time() >= HARDFORK_0_4_TIME)
       d.remove(d.get_account_auth_platform_object_by_account_platform(o.uid, o.platform));

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result account_update_proxy_evaluator::do_evaluate( const account_update_proxy_operation& op )
{ try {
   database& d = db();
   account_stats = &d.get_account_statistics_by_uid( op.voter );

   FC_ASSERT( account_stats->can_vote == true, "This account can not vote" );

   const auto& global_params = d.get_global_properties().parameters;
   FC_ASSERT( account_stats->core_balance >= global_params.min_governance_voting_balance,
              "Need more balance to be able to vote: have ${b}, need ${r}",
              ("b",d.to_pretty_core_string(account_stats->core_balance))
              ("r",d.to_pretty_core_string(global_params.min_governance_voting_balance)) );

   if( op.proxy != GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID )
   {
      const auto& proxy_account_stats = d.get_account_statistics_by_uid( op.proxy );
      FC_ASSERT( proxy_account_stats.is_voter, "Proxy should already be a voter" );

      proxy_voter_obj = d.find_voter( op.proxy, proxy_account_stats.last_voter_sequence );
      FC_ASSERT( proxy_voter_obj != nullptr, "proxy voter should exist" );

      bool proxy_still_valid = d.check_voter_valid( *proxy_voter_obj, true );
      FC_ASSERT( proxy_still_valid, "proxy voter should still be valid" );
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

   if( voter_obj == nullptr || voter_obj->proxy_uid == GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID )
      FC_ASSERT( op.proxy != GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID,
                 "Already voting by self, or was voting with a proxy but the proxy is no longer valid" );
   else
   {
      current_proxy_voter_obj = d.find_voter( voter_obj->proxy_uid, voter_obj->proxy_sequence );
      FC_ASSERT( current_proxy_voter_obj != nullptr, "proxy voter should exist" );
      bool current_proxy_still_valid = d.check_voter_valid( *current_proxy_voter_obj, true );
      if( !current_proxy_still_valid )
      {
         FC_ASSERT( op.proxy != GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID,
                    "Already voting by self, or was voting with a proxy but the proxy is no longer valid" );
         invalid_current_proxy_voter_obj = current_proxy_voter_obj;
         current_proxy_voter_obj = nullptr;
      }
      else // current proxy is still valid
         FC_ASSERT( op.proxy != voter_obj->proxy_uid, "Should change something" );
   }

   // check for proxy loop
   if( voter_obj != nullptr && proxy_voter_obj != nullptr )
   {
      const auto max_level = global_params.max_governance_voting_proxy_level;
      const voter_object* new_proxy = proxy_voter_obj;
      for( uint8_t level = 0; level < max_level && new_proxy->proxy_uid != GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID; ++level )
      {
         FC_ASSERT( new_proxy->proxy_uid != voter_obj->uid || new_proxy->proxy_sequence != voter_obj->sequence,
                    "Proxy loop detected." );
         if( level + 1 >= max_level )
            break;
         new_proxy = d.find_voter( new_proxy->proxy_uid, new_proxy->proxy_sequence );
         FC_ASSERT( new_proxy != nullptr, "proxy voter should exist" );
      }
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result account_update_proxy_evaluator::do_apply( const account_update_proxy_operation& op )
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

   if( voter_obj != nullptr ) // voter already exists
   {
      // clear current votes
      d.clear_voter_votes( *voter_obj );
      // deal with current proxy
      if( current_proxy_voter_obj != nullptr ) // current proxy is still valid
      {
         d.modify( *current_proxy_voter_obj, [&](voter_object& v) {
            v.proxied_voters -= 1;
         });
      }
      // setup new proxy
      if( op.proxy == GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID ) // self
      {
         d.modify( *voter_obj, [&](voter_object& v) {
            v.proxy_uid                 = GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID;
            v.proxy_sequence            = 0;
            v.proxy_last_vote_block[0]  = head_block_num;
            v.effective_last_vote_block = head_block_num;
         });
      }
      else // proxy to a voter
      {
         d.modify( *voter_obj, [&](voter_object& v) {
            v.proxy_uid                 = op.proxy;
            v.proxy_sequence            = proxy_voter_obj->sequence;
            v.proxy_last_vote_block[0]  = head_block_num;
            v.effective_last_vote_block = head_block_num;
         });

         // proxied votes
         vector<share_type> delta( max_level ); // [ self, proxied_level1, proxied_level2, ... ]
         delta[0] = voter_obj->effective_votes;
         for( uint8_t i = 1; i < max_level; ++i )
         {
            delta[i] = voter_obj->proxied_votes[i-1];
         }
         d.adjust_voter_proxy_votes( *voter_obj, delta, false );
      }
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

         v.proxy_uid         = op.proxy;
         if( op.proxy != GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID )
            v.proxy_sequence = proxy_voter_obj->sequence;
         // else proxy_sequence = 0; // default

         //v.proxied_voters    = 0; // default
         v.proxied_votes.resize( max_level, 0 ); // [ level1, level2, ... ]
         v.proxy_last_vote_block.resize( max_level+1, 0 ); // [ self, proxy, proxy->proxy, ... ]
         v.proxy_last_vote_block[0] = head_block_num;

         v.effective_last_vote_block         = head_block_num;

         //v.number_of_witnesses_voted = 0; // default
      });
   }

   if( op.proxy != GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID )
   {
      d.modify( *proxy_voter_obj, [&](voter_object& v) {
         v.proxied_voters += 1;
      });
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result account_enable_allowed_assets_evaluator::do_evaluate( const account_enable_allowed_assets_operation& o )
{ try {
   database& d = db();
   FC_ASSERT( d.head_block_time() >= HARDFORK_0_3_TIME,
              "Can only use account_enable_allowed_assets_operation after HARDFORK_0_3_TIME" );

   acnt = &d.get_account_by_uid( o.account );

   FC_ASSERT( o.enable != acnt->allowed_assets.valid(), "Should change something" );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result account_enable_allowed_assets_evaluator::do_apply( const account_enable_allowed_assets_operation& o )
{ try {
   database& d = db();

   d.modify( *acnt, [&d](account_object& a){
      if( a.allowed_assets.valid() )
         a.allowed_assets.reset();
      else
      {
         a.allowed_assets = flat_set<asset_aid_type>();
         a.allowed_assets->insert( GRAPHENE_CORE_ASSET_AID );
      }
      a.last_update_time = d.head_block_time();
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result account_update_allowed_assets_evaluator::do_evaluate( const account_update_allowed_assets_operation& o )
{ try {
   database& d = db();
   FC_ASSERT( d.head_block_time() >= HARDFORK_0_3_TIME,
              "Can only use account_update_allowed_assets_operation after HARDFORK_0_3_TIME" );

   acnt = &d.get_account_by_uid( o.account );

   FC_ASSERT( acnt->allowed_assets.valid(), "Account did not enable allowed_assets, can not update" );

   for( const auto aid : o.assets_to_remove )
   {
      FC_ASSERT( acnt->allowed_assets->find( aid ) != acnt->allowed_assets->end(),
                 "Account did not allow asset ${a}, can not remove", ("a",aid) );
      //get_asset_by_aid( aid ); // no need to check
   }
   for( const auto aid : o.assets_to_add )
   {
      FC_ASSERT( acnt->allowed_assets->find( aid ) == acnt->allowed_assets->end(),
                 "Account already allowed asset ${a}, can not add", ("a",aid) );
      d.get_asset_by_aid( aid );
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result account_update_allowed_assets_evaluator::do_apply( const account_update_allowed_assets_operation& o )
{ try {
   database& d = db();

   d.modify( *acnt, [&d,&o](account_object& a){
      for( const auto aid : o.assets_to_remove )
         a.allowed_assets->erase( aid );
      for( const auto aid : o.assets_to_add )
         a.allowed_assets->insert( aid );
      a.last_update_time = d.head_block_time();
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result account_whitelist_evaluator::do_evaluate(const account_whitelist_operation& o)
{ try {
   database& d = db();

   listed_account = d.find_account_by_uid( o.account_to_list );
   if( !d.get_global_properties().parameters.allow_non_member_whitelists )
      FC_ASSERT(d.get_account_by_uid( o.authorizing_account ).is_lifetime_member());

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
   d.modify( d.get_account_by_uid( o.authorizing_account ), [&]( account_object& a ) {
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

} } // graphene::chain
