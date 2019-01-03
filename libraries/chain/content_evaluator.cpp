/*
 * Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
 */
#include <graphene/chain/content_evaluator.hpp>
#include <graphene/chain/content_object.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/is_authorized_asset.hpp>
#include <graphene/chain/protocol/chain_parameters.hpp>
#include <boost/multiprecision/cpp_int.hpp>

namespace graphene { namespace chain {

void_result platform_create_evaluator::do_evaluate( const platform_create_operation& op )
{ try {
   database& d = db();

   FC_ASSERT( ( d.head_block_time() >= HARDFORK_0_2_TIME ) || ( d.head_block_num() <= 4570000 ), "Can only be create platform after HARDFORK_0_2_TIME" );
   
   account_stats = &d.get_account_statistics_by_uid( op.account );
   account_obj = &d.get_account_by_uid( op.account );

   const auto& global_params = d.get_global_properties().parameters;
   
   if( d.head_block_num() > 0 )
      FC_ASSERT( op.pledge.amount >= global_params.platform_min_pledge,
                 "Insufficient pledge: provided ${p}, need ${r}",
                 ("p",d.to_pretty_string(op.pledge))
                 ("r",d.to_pretty_core_string(global_params.platform_min_pledge)) );

   auto available_balance = account_stats->core_balance
                          - account_stats->core_leased_out
                          - account_stats->total_committee_member_pledge
                          - account_stats->total_witness_pledge;
   FC_ASSERT( available_balance >= op.pledge.amount,
              "Insufficient Balance: account ${a}'s available balance of ${b} is less than required ${r}",
              ("a",op.account)
              ("b",d.to_pretty_core_string(available_balance))
              ("r",d.to_pretty_string(op.pledge)) );

   const platform_object* maybe_found = d.find_platform_by_owner( op.account );
   FC_ASSERT( maybe_found == nullptr, "This account already has a platform" );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type platform_create_evaluator::do_apply( const platform_create_operation& op )
{ try {
   database& d = db();
   const auto& global_params = d.get_global_properties().parameters;

   const auto& new_platform_object = d.create<platform_object>( [&]( platform_object& pf ){
      pf.owner                = op.account;
      pf.name                 = op.name;
      pf.sequence            = account_stats->last_platform_sequence + 1;
      pf.pledge              = op.pledge.amount.value;
      pf.url                 = op.url;
      pf.extra_data          = op.extra_data;
      pf.create_time         = d.head_block_time();

      pf.pledge_last_update  = d.head_block_time();

      pf.average_pledge_last_update       = d.head_block_time();
      if( pf.pledge > 0 )
         pf.average_pledge_next_update_block = d.head_block_num() + global_params.platform_avg_pledge_update_interval;
      else 
         pf.average_pledge_next_update_block = -1;

   });

   d.modify( *account_obj, [&]( account_object& a) {
         a.is_full_member = true;
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

   FC_ASSERT( ( d.head_block_time() >= HARDFORK_0_2_TIME ) || ( d.head_block_num() <= 4570000 ), "Can only be update platform after HARDFORK_0_2_TIME" );

   account_stats = &d.get_account_statistics_by_uid( op.account );
   platform_obj   = &d.get_platform_by_owner( op.account );

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
                                - account_stats->core_leased_out
                                - account_stats->total_committee_member_pledge
                                - account_stats->total_witness_pledge;
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
   else // When updating the platform, you have to check the mortgage
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
   const account_object* account_obj = &d.get_account_by_uid( op.account );

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
         pfo.is_valid = false; // Processing will be delayed
      });
      d.modify( *account_obj, [&]( account_object& acc ){
         acc.is_full_member = false;
      });
   }
   else // change pledge
   {
      // update account stats
      share_type delta = op.new_pledge->amount.value - platform_obj->pledge;
      if( delta > 0 ) // Increase the mortgage
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
      else // Reduce the mortgage
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
      d.update_platform_avg_pledge( *platform_obj );
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
      const platform_object& po = d.get_platform_by_owner( uid );
      platform_to_remove.push_back( &po );
   }
   for( const auto uid : op.platform_to_add )
   {
      const platform_object& po = d.get_platform_by_owner( uid );
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
         const platform_object* platform = d.find_platform_by_owner( itr->platform_owner );
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
         d.adjust_platform_votes( *platform_to_remove[i], -total_votes );
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
         o.voter_uid         = op.voter;
         o.voter_sequence    = voter_obj->sequence;
         o.platform_owner    = platform_to_add[i]->owner;
         o.platform_sequence = platform_to_add[i]->sequence;
      });
      if( total_votes > 0 )
         d.adjust_platform_votes( *platform_to_add[i], total_votes );
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }



void_result post_evaluator::do_evaluate( const post_operation& op )
{ try {

   const database& d = db();
   account_stats = &d.get_account_statistics_by_uid( op.poster );

   auto platform = d.get_platform_by_owner( op.platform ); // make sure pid exists
   const account_object* poster_account = &d.get_account_by_uid( op.poster );

   FC_ASSERT( ( poster_account != nullptr && poster_account->can_post ),
              "poster ${uid} is not allowed to post.",
              ("uid",op.poster) );

   FC_ASSERT( ( account_stats->last_post_sequence + 1 ) == op.post_pid ,
              "post_pid ${pid} is invalid.",
              ("pid", op.post_pid) );

   if( op.origin_post_pid.valid() ) // is Reprint
   {
      const account_statistics_object* origin_account_stats = &d.get_account_statistics_by_uid( *op.origin_poster );

      FC_ASSERT( origin_account_stats != nullptr,
                 "the ${uid} origin poster not exists.",
                 ("uid", op.origin_poster) );

      FC_ASSERT( origin_account_stats->last_post_sequence >= *op.origin_post_pid,
                 "the ${pid} origin post not exists.",
                 ("pid", op.origin_post_pid) );
   }

   account_uid_type sign_account = sigs.real_secondary_uid(op.poster, 1);
   if (sign_account == op.platform)
   {
       auto auth_data = account_stats->prepaids_for_platform.find(sign_account);
       if (auth_data != account_stats->prepaids_for_platform.end())
       {
           sign_platform_uid = sign_account;
       }
   }

   if (op.extensions.valid() && d.head_block_time() >= HARDFORK_0_4_TIME)
   {
	   for (auto ext_iter = op.extensions->begin(); ext_iter != op.extensions->end(); ext_iter++)
	   {
		   if (ext_iter->which() == post_operation::extension_parameter::tag<post_operation::ext>::value)
		   {
			   ext = &(ext_iter->get<post_operation::ext>());
			   if (ext->post_type == post_operation::Post_Type::Post_Type_Comment)
			   {
                   d.get_platform_by_owner(*op.origin_platform); // make sure pid exists
                   d.get_account_by_uid(*op.origin_poster); // make sure uid exists
                   const post_object& origin_post = d.get_post_by_platform(*op.origin_platform, *op.origin_poster, *op.origin_post_pid); // make sure pid exists
                   FC_ASSERT((origin_post.permission_flags & post_object::Post_Permission_Comment)>0, 
                              "post_object ${p} not allowed to comment.", 
                              ("p", op.origin_post_pid));
                   FC_ASSERT((poster_account->can_reply), 
                              "poster ${uid} is not allowed to reply.", 
                              ("uid", op.poster));

                   auto auth_data = account_stats->prepaids_for_platform.find(op.platform);
                   FC_ASSERT(auth_data != account_stats->prepaids_for_platform.end(), 
                             "platform ${p} not included in account ${a} `s prepaids_for_platform. ",
                             ("p", op.platform)("a", op.poster));
                   FC_ASSERT((auth_data->second.permission_flags & account_statistics_object::Platform_Permission_Comment) > 0,
                              "the comment permission of platform ${p} authorized by account ${a} is invalid. ",
                              ("p", op.platform)("a", op.poster));
			   }
			   if (ext->post_type == post_operation::Post_Type::Post_Type_forward
				   || ext->post_type == post_operation::Post_Type::Post_Type_forward_And_Modify)
			   {
                   d.get_platform_by_owner(*op.origin_platform); // make sure pid exists
                   d.get_account_by_uid(*op.origin_poster); // make sure uid exists
                   const post_object& origin_post = d.get_post_by_platform(*op.origin_platform, *op.origin_poster, *op.origin_post_pid); // make sure pid exists
                   FC_ASSERT((origin_post.permission_flags & post_object::Post_Permission_Forward)>0, 
                             "post_object ${p} not allowed to forward.", 
                             ("p", op.origin_post_pid));
                   FC_ASSERT(origin_post.forward_price.valid(),
                             "post ${p} is not allowed to forward",
                             ("p", op.origin_post_pid));
				   
                   auto auth_data = account_stats->prepaids_for_platform.find(op.platform);
                   FC_ASSERT(auth_data != account_stats->prepaids_for_platform.end(), 
                             "platform ${p} not included in account ${a} `s prepaids_for_platform. ",
                             ("p",op.platform)("a",op.poster));
                   FC_ASSERT((auth_data->second.permission_flags & account_statistics_object::Platform_Permission_Forward)>0, 
                              "the proxy_post of platform ${p} authorized by account ${a} is invalid. ",
                              ("p", op.platform)("a", op.poster));
                   FC_ASSERT(account_stats->prepaid >= *origin_post.forward_price, 
                             "Insufficient balance: unable to forward, because the account ${a} `s prepaid [${c}] is less then needed [${n}]. ",
                             ("c", (account_stats->prepaid))("a", op.poster)("n", origin_post.forward_price));

                   if (auth_data->second.max_limit < GRAPHENE_MAX_PLATFORM_LIMIT_PREPAID && sign_platform_uid.valid())
                   {
                       share_type usable_prepaid = account_stats->get_auth_platform_usable_prepaid(*sign_platform_uid);
                       FC_ASSERT(usable_prepaid >= *origin_post.forward_price,
                                 "Insufficient balance: unable to forward, because the prepaid [${c}] of platform ${p} authorized by account ${a} is less then needed [${n}]. ",
                                 ("c", (usable_prepaid))("p", *sign_platform_uid)("a", op.poster)("n", *origin_post.forward_price));
                   }   
			   }
		   }
	   }
   }

   return void_result();

}  FC_CAPTURE_AND_RETHROW( (op) ) }

typedef boost::multiprecision::uint128_t uint128_t;

object_id_type post_evaluator::do_apply( const post_operation& o )
{ try {
      database& d = db();

      d.modify( *account_stats, [&](account_statistics_object& s) {
         s.last_post_sequence += 1;
      });

      if (ext && d.head_block_time() >= HARDFORK_0_4_TIME)
	  {
          if (ext->post_type == post_operation::Post_Type::Post_Type_forward
              || ext->post_type == post_operation::Post_Type::Post_Type_forward_And_Modify)
          {
              const post_object& origin_post = d.get_post_by_platform(*o.origin_platform, *o.origin_poster, *o.origin_post_pid);
              share_type forwardprice = *(origin_post.forward_price);
              d.modify(*account_stats, [&](account_statistics_object& obj)
              {
                  if (sign_platform_uid.valid()) // signed by platform , then add auth cur_used
                  {
                      auto iter = obj.prepaids_for_platform.find(o.platform);
                      iter->second.cur_used += forwardprice;
                  }
                  obj.prepaid -= forwardprice;
              });

              const post_object* post = &d.get_post_by_platform(*o.origin_platform, *o.origin_poster, *o.origin_post_pid);
              uint128_t amount(forwardprice.value);
              uint128_t surplus = amount;
              for (auto iter : post->receiptors)
              {
                  if (iter.first == post->platform)
                      continue;
                  uint128_t temp = (amount*(iter.second.cur_ratio)) / GRAPHENE_100_PERCENT;
                  surplus -= temp;
                  d.modify(d.get_account_statistics_by_uid(iter.first), [&](account_statistics_object& obj)
                  {
                      obj.prepaid += temp.convert_to<int64_t>();
                  });
              }
              d.modify(d.get_account_statistics_by_uid(post->platform), [&](account_statistics_object& obj)
              {
                  obj.prepaid += surplus.convert_to<int64_t>();
              });
          }
	  }

      const auto& new_post_object = d.create<post_object>( [&]( post_object& obj )
      {
            obj.platform         = o.platform;
            obj.poster           = o.poster;
            obj.post_pid         = o.post_pid;
            obj.origin_poster    = o.origin_poster;
            obj.origin_post_pid  = o.origin_post_pid;
            obj.origin_platform  = o.origin_platform;
            obj.hash_value       = o.hash_value;
            obj.extra_data       = o.extra_data;
            obj.title            = o.title;
            obj.body             = o.body;
            obj.create_time      = d.head_block_time();
            obj.last_update_time = d.head_block_time();
            obj.score_settlement = false;

            if (d.head_block_time() >= HARDFORK_0_4_TIME)
            {
                bool need_init_receiptors = true;
                if (ext)
                {
                    if (ext->forward_price.valid())
                        obj.forward_price = *(ext->forward_price);
                    if (ext->receiptors.valid())
                    {
                        map<account_uid_type, Recerptor_Parameter> map_receiptor = *(ext->receiptors);
                        if (map_receiptor.size() > 0)
                        {
                            need_init_receiptors = false;
                            obj.receiptors = map_receiptor;
                        }
                    }
                    if (ext->license_lid.valid())
                    {
                        obj.license_lid = *(ext->license_lid);
                    }
                    obj.permission_flags = ext->permission_flags;
                }
                if (need_init_receiptors){
                    map<account_uid_type, Recerptor_Parameter> map_receiptors;
                    map_receiptors.insert(make_pair(o.platform, Recerptor_Parameter{ GRAPHENE_DEFAULT_PLATFORM_RECERPTS_RATIO, false, 0, 0 }));
                    map_receiptors.insert(make_pair(o.poster, Recerptor_Parameter{ GRAPHENE_100_PERCENT - GRAPHENE_DEFAULT_PLATFORM_RECERPTS_RATIO, false, 0, 0 }));
                    obj.receiptors = map_receiptors;
                }
            }
      } );
      return new_post_object.id;
   
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result post_update_evaluator::do_evaluate( const operation_type& op )
{ try {
   const database& d = db();

   d.get_platform_by_owner( op.platform ); // make sure pid exists
   const account_object* poster_account = &d.get_account_by_uid( op.poster );
   const account_statistics_object* account_stats = &d.get_account_statistics_by_uid( op.poster );

   if (op.hash_value.valid() || op.extra_data.valid() || op.title.valid() || op.body.valid())
   {
	   FC_ASSERT((poster_account != nullptr && poster_account->can_post), "poster ${uid} is not allowed to post.", ("uid", op.poster));

	   FC_ASSERT((account_stats != nullptr && account_stats->last_post_sequence >= op.post_pid), "post_pid ${pid} is invalid.", ("pid", op.post_pid));

	   post = d.find_post_by_platform(op.platform, op.poster, op.post_pid);

	   FC_ASSERT(post != nullptr, "post ${pid} is invalid.", ("pid", op.post_pid));
   }

   if (op.extensions.valid() && d.head_block_time() >= HARDFORK_0_4_TIME)
   {
	   for (auto ext_iter = op.extensions->begin(); ext_iter != op.extensions->end(); ext_iter++)
	   {
		   if (ext_iter->which() == post_update_operation::extension_parameter::tag<post_update_operation::ext>::value)
		   {
			   ext = &(ext_iter->get<post_update_operation::ext>());
			   if (ext->receiptor.valid() && ext->buyout_ratio.valid())
			   {
				   post = d.find_post_by_platform(op.platform, op.poster, op.post_pid);
				   FC_ASSERT(post != nullptr, "post ${pid} is invalid.", ("pid", op.post_pid));
				   auto iter = post->receiptors.find(*(ext->receiptor));
				   FC_ASSERT(iter != post->receiptors.end(), 
                             "receiptor:${r} not found.", 
                             ("r", *(ext->receiptor)));
				   FC_ASSERT(iter->second.cur_ratio >= *(ext->buyout_ratio), 
                             "the ratio ${r} of receiptor ${p} is less then sell ${sp} .",
					         ("r", iter->second.cur_ratio)("p", *(ext->receiptor))("sp", *(ext->buyout_ratio)));
			   }
		   }
	   }
   }

   return void_result();

} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type post_update_evaluator::do_apply( const operation_type& o )
{ try {
   database& d = db();
   
   d.modify( *post, [&]( post_object& obj )
      {
         if( o.hash_value.valid() )
            obj.hash_value       = *o.hash_value;
         if( o.extra_data.valid() )
            obj.extra_data       = *o.extra_data;
         if( o.title.valid() )
            obj.title            = *o.title;
         if( o.body.valid() )
            obj.body             = *o.body;

         if (ext && d.head_block_time() >= HARDFORK_0_4_TIME)
		 {
             if (ext->forward_price.valid())
             {
                 obj.forward_price = *(ext->forward_price);
             }
             if (ext->receiptor.valid())
             {
                 auto iter = obj.receiptors.find(*(ext->receiptor));
                 if (ext->to_buyout.valid())
                     iter->second.to_buyout = *(ext->to_buyout);
                 if (ext->buyout_ratio.valid())
                     iter->second.buyout_ratio = *(ext->buyout_ratio);
                 if (ext->buyout_price.valid())
                     iter->second.buyout_price = *(ext->buyout_price);
                 if (ext->buyout_expiration.valid())
                     iter->second.buyout_expiration = *(ext->buyout_expiration);
             }
             if (ext->license_lid.valid())
             {
                 obj.license_lid = *(ext->license_lid);
             }
             if (ext->permission_flags.valid())
             {
                 obj.permission_flags = *(ext->permission_flags);
             }
		 }

         obj.last_update_time = d.head_block_time();
      } );
      return post->id;

} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result score_create_evaluator::do_evaluate(const operation_type& op)
{
	try {
		const database& d = db();
        FC_ASSERT(d.head_block_time() >= HARDFORK_0_4_TIME, "Can only create_score after HARDFORK_0_4_TIME");

		const auto& global_params = d.get_global_properties().parameters.get_award_params();
		auto from_account = d.get_account_by_uid(op.from_account_uid);// make sure uid exists
        const post_object& origin_post = d.get_post_by_platform(op.platform, op.poster, op.post_pid);// make sure pid exists
        FC_ASSERT((origin_post.permission_flags & post_object::Post_Permission_Liked) > 0, "post_object ${p} not allowed to liked.", ("p", op.post_pid));
		FC_ASSERT((from_account.can_rate), "poster ${uid} is not allowed to appraise.", ("uid", op.poster));
		FC_ASSERT(op.csaf <= global_params.max_csaf_per_approval, "The score_create_operation`s member points is over the maximum limit");

        const account_statistics_object* account_stats = &d.get_account_statistics_by_uid(op.from_account_uid);
        auto auth_data = account_stats->prepaids_for_platform.find(op.platform);
        FC_ASSERT(auth_data != account_stats->prepaids_for_platform.end(), 
                  "platform ${p} not included in account ${a} `s prepaids_for_platform. ",
                  ("p", op.platform)("a", op.poster));
        FC_ASSERT((auth_data->second.permission_flags & account_statistics_object::Platform_Permission_Liked)>0, 
                  "the liked permisson of platform ${p} authorized by account ${a} is invalid. ",
                   ("p", op.platform)("a", op.poster));
		FC_ASSERT(account_stats->csaf >= op.csaf,
                  "Insufficient csaf: unable to score, because account: ${f} `s member points [${c}] is less then needed [${n}]",
			      ("f",op.from_account_uid)("c",account_stats->csaf)("n",op.csaf));

        FC_ASSERT(d.find_score(op.platform, op.poster, op.post_pid, op.from_account_uid)==nullptr, "only score a post once");

		const auto& apt_idx = d.get_index_type<active_post_index>().indices().get<by_post_pid>();
        uint64_t sequence = d.get_dynamic_global_properties().current_active_post_sequence;
		auto apt_itr = apt_idx.find(std::make_tuple(op.platform, op.poster, op.post_pid, sequence));
		if (apt_itr != apt_idx.end())
		{
			active_post = &(*apt_itr);
			FC_ASSERT(active_post->platform == op.platform, "platform should be the same.");
			FC_ASSERT(active_post->poster == op.poster, "poster should be the same.");
		}

		return void_result();
	}FC_CAPTURE_AND_RETHROW((op))
}

object_id_type score_create_evaluator::do_apply(const operation_type& op)
{
	try {
		database& d = db();

		const account_statistics_object* account_stats = &d.get_account_statistics_by_uid(op.from_account_uid);
		d.modify(*account_stats, [&](account_statistics_object& s) {
			s.csaf -= op.csaf;
		});
		const auto& new_score_object = d.create<score_object>([&](score_object& obj)
		{
			obj.from_account_uid = op.from_account_uid;
            obj.platform         = op.platform;
            obj.poster           = op.poster;
			obj.post_pid         = op.post_pid;
			obj.score            = op.score;
			obj.csaf             = op.csaf;
			obj.create_time      = d.head_block_time();
		});

		if (active_post)
		{
			d.modify(*active_post, [&](active_post_object& s) {
				s.total_amount += op.csaf;
				s.scores.push_back(new_score_object.id);
			});
		}
		else
		{
            const post_object* post = &d.get_post_by_platform(op.platform, op.poster, op.post_pid);
            time_point_sec expiration_time = post->create_time;
            if ((expiration_time += d.get_global_properties().parameters.get_award_params().post_award_expiration ) >= d.head_block_time())
            {
                const dynamic_global_property_object& dpo = d.get_dynamic_global_properties();
                d.create<active_post_object>([&](active_post_object& obj)
                {
                    obj.platform = op.platform;
                    obj.poster = op.poster;
                    obj.post_pid = op.post_pid;
                    obj.total_amount = op.csaf;
                    obj.period_sequence = dpo.current_active_post_sequence;
                    obj.scores.push_back(new_score_object.id);
                });
            }
		}

		return new_score_object.id;
	}FC_CAPTURE_AND_RETHROW((op))
}

void_result reward_evaluator::do_evaluate(const operation_type& op)
{
	try {
        const database& d = db();
        FC_ASSERT(d.head_block_time() >= HARDFORK_0_4_TIME, "Can only be reward after HARDFORK_0_4_TIME");
		
		d.get_account_by_uid(op.from_account_uid);// make sure uid exists
        const post_object& origin_post = d.get_post_by_platform(op.platform, op.poster, op.post_pid);// make sure pid exists
        FC_ASSERT((origin_post.permission_flags & post_object::Post_Permission_Reward) > 0, "post_object ${p} not allowed to reward.", ("p", op.post_pid));
		const account_statistics_object* account_stats = &d.get_account_statistics_by_uid(op.from_account_uid);

		const account_object* from_account = &d.get_account_by_uid(op.from_account_uid);
		const asset_object&   transfer_asset_object = d.get_asset_by_aid(op.amount.asset_id);
		validate_authorized_asset(d, *from_account, transfer_asset_object, "'from' ");

		if (transfer_asset_object.is_transfer_restricted())
		{
			GRAPHENE_ASSERT(
				from_account->uid == transfer_asset_object.issuer,
				transfer_restricted_transfer_asset,
				"Asset {asset} has transfer_restricted flag enabled.",
				("asset", op.amount.asset_id)
			);
		}

		if (op.amount.amount > 0)
		{
			const auto& from_balance = d.get_balance(*from_account, transfer_asset_object);
			bool sufficient_balance = from_balance.amount >= op.amount.amount;
			FC_ASSERT(sufficient_balance, 
                      "Insufficient balance: unable to reward, because account: ${f} `s balance [${c}] is less then needed [${n}]",
				      ("f", op.from_account_uid)("c", from_balance.amount)("n", op.amount.amount));
		}

		const auto& apt_idx = d.get_index_type<active_post_index>().indices().get<by_post_pid>();
        uint64_t sequence = d.get_dynamic_global_properties().current_active_post_sequence;
        auto apt_itr = apt_idx.find(std::make_tuple(op.platform, op.poster, op.post_pid, sequence));
		if (apt_itr != apt_idx.end())
		{
			active_post = &(*apt_itr);
			FC_ASSERT(active_post->platform == op.platform, "platform should be the same.");
			FC_ASSERT(active_post->poster == op.poster, "poster should be the same.");
		}

		return void_result();
	}FC_CAPTURE_AND_RETHROW((op))
}

void_result reward_evaluator::do_apply(const operation_type& op)
{
	try {
		database& d = db();

		const account_object* from_account = &d.get_account_by_uid(op.from_account_uid);
		d.adjust_balance(*from_account, -op.amount);

		const post_object* post = &d.get_post_by_platform(op.platform, op.poster, op.post_pid);
		uint128_t amount(op.amount.amount.value);
		uint128_t surplus = amount;
		asset ast(share_type(0), op.amount.asset_id);
		for (auto iter : post->receiptors)
		{
			if (iter.first == post->platform)
				continue;
			uint128_t temp = (amount*(iter.second.cur_ratio)) / 10000;
			ast.amount = temp.convert_to<int64_t>();
			surplus -= temp;
			d.adjust_balance(iter.first, ast);
		}
        ast.amount = surplus.convert_to<int64_t>();
		d.adjust_balance(post->platform, ast);

		if (active_post)
		{
			d.modify(*active_post, [&](active_post_object& s) {
				if (s.total_rewards.find(op.amount.asset_id) != s.total_rewards.end())
					s.total_rewards.at(op.amount.asset_id) += op.amount.amount;
				else
					s.total_rewards.emplace(op.amount.asset_id, op.amount.amount);
			});
		}
		else
		{
            time_point_sec expiration_time = post->create_time;
            if ((expiration_time += d.get_global_properties().parameters.get_award_params().post_award_expiration) >= d.head_block_time())
            {
                const dynamic_global_property_object& dpo = d.get_dynamic_global_properties();
                d.create<active_post_object>([&](active_post_object& obj)
                {
                    obj.platform = op.platform;
                    obj.poster = op.poster;
                    obj.post_pid = op.post_pid;
                    obj.total_amount = 0;
                    obj.period_sequence = dpo.current_active_post_sequence;
                    obj.total_rewards.emplace(op.amount.asset_id, op.amount.amount);
                });
            }
		}

		return void_result();
	}FC_CAPTURE_AND_RETHROW((op))
}

void_result reward_proxy_evaluator::do_evaluate(const operation_type& op)
{
    try {
        const database& d = db();
        FC_ASSERT(d.head_block_time() >= HARDFORK_0_4_TIME, "Can only be reward_proxy after HARDFORK_0_4_TIME");

        d.get_account_by_uid(op.from_account_uid);// make sure uid exists
        const post_object& origin_post = d.get_post_by_platform(op.platform, op.poster, op.post_pid);// make sure pid exists
        FC_ASSERT((origin_post.permission_flags & post_object::Post_Permission_Reward) > 0, "post_object ${p} not allowed to reward.", ("p", op.post_pid));
        const account_statistics_object* account_stats = &d.get_account_statistics_by_uid(op.from_account_uid);

        account_uid_type sign_account = sigs.real_secondary_uid(op.from_account_uid, 1);
        if (sign_account == op.platform)
        {
            auto auth_data = account_stats->prepaids_for_platform.find(sign_account);
            if (auth_data != account_stats->prepaids_for_platform.end())
            {
                sign_platform_uid = sign_account;
            }
        }

        auto auth_data = account_stats->prepaids_for_platform.find(op.platform);
        FC_ASSERT(auth_data != account_stats->prepaids_for_platform.end(), 
                  "platform ${p} not included in account ${a} `s prepaids_for_platform. ",
                  ("p", op.platform)("a", op.poster));
        FC_ASSERT((auth_data->second.permission_flags & account_statistics_object::Platform_Permission_Reward)>0, 
                  "the reward permisson of platform ${p} authorized by account ${a} is invalid. ",
                  ("p", op.platform)("a", op.poster));
        FC_ASSERT(account_stats->prepaid >= op.amount, 
                  "Insufficient balance: unable to reward, because the account ${a} `s prepaid [${c}] is less then needed [${n}]. ",
                  ("c", (account_stats->prepaid))("a", op.poster)("n", op.amount));
        if (auth_data->second.max_limit < GRAPHENE_MAX_PLATFORM_LIMIT_PREPAID && sign_platform_uid.valid())
        {
            share_type usable_prepaid = account_stats->get_auth_platform_usable_prepaid(*sign_platform_uid);
            FC_ASSERT(usable_prepaid >= op.amount,
                      "Insufficient balance: unable to reward, because the prepaid [${c}] of platform ${p} authorized by account ${a} is less then needed [${n}]. ",
                      ("c", usable_prepaid)("p", *sign_platform_uid)("a", op.poster)("n", op.amount));
        }   

        const auto& apt_idx = d.get_index_type<active_post_index>().indices().get<by_post_pid>();
        uint64_t sequence = d.get_dynamic_global_properties().current_active_post_sequence;
        auto apt_itr = apt_idx.find(std::make_tuple(op.platform, op.poster, op.post_pid, sequence));
        if (apt_itr != apt_idx.end())
        {
            active_post = &(*apt_itr);
            FC_ASSERT(active_post->platform == op.platform, "platform should be the same.");
            FC_ASSERT(active_post->poster == op.poster, "poster should be the same.");
        }

        return void_result();
    }FC_CAPTURE_AND_RETHROW((op))
}

void_result reward_proxy_evaluator::do_apply(const operation_type& op)
{
    try {
        database& d = db();
        const account_statistics_object* account_stats = &d.get_account_statistics_by_uid(op.from_account_uid);
        d.modify(*account_stats, [&](account_statistics_object& obj)
        {
            if (sign_platform_uid.valid())
            {
                auto iter = obj.prepaids_for_platform.find(op.platform); // signed by platform , then add auth cur_used
                iter->second.cur_used += op.amount;
            }
            obj.prepaid -= op.amount;
        });

        const post_object* post = &d.get_post_by_platform(op.platform, op.poster, op.post_pid);
        uint128_t amount(op.amount.value);
        uint128_t surplus = amount;
        for (auto iter : post->receiptors)
        {
            if (iter.first == post->platform)
                continue;
            uint128_t temp = (amount*(iter.second.cur_ratio)) / GRAPHENE_100_PERCENT;
            surplus -= temp;
            d.modify(d.get_account_statistics_by_uid(iter.first), [&](account_statistics_object& obj)
            {
                obj.prepaid += temp.convert_to<int64_t>();
            });
        }
        d.modify(d.get_account_statistics_by_uid(post->platform), [&](account_statistics_object& obj)
        {
            obj.prepaid += surplus.convert_to<int64_t>();
        });

        if (active_post)
        {
            d.modify(*active_post, [&](active_post_object& s) {
                if (s.total_rewards.find(GRAPHENE_CORE_ASSET_AID) != s.total_rewards.end())
                    s.total_rewards.at(GRAPHENE_CORE_ASSET_AID) += op.amount;
                else
                    s.total_rewards.emplace(GRAPHENE_CORE_ASSET_AID, op.amount);
            });
        }
        else
        {
            time_point_sec expiration_time = post->create_time;
            if ((expiration_time += d.get_global_properties().parameters.get_award_params().post_award_expiration) >= d.head_block_time())
            {
                const dynamic_global_property_object& dpo = d.get_dynamic_global_properties();
                d.create<active_post_object>([&](active_post_object& obj)
                {
                    obj.platform = op.platform;
                    obj.poster = op.poster;
                    obj.post_pid = op.post_pid;
                    obj.total_amount = 0;
                    obj.period_sequence = dpo.current_active_post_sequence;
                    obj.total_rewards.emplace(GRAPHENE_CORE_ASSET_AID, op.amount);
                });
            }
        }

        return void_result();
    }FC_CAPTURE_AND_RETHROW((op))
}

void_result buyout_evaluator::do_evaluate(const operation_type& op)
{
	try {
		database& d = db();
        FC_ASSERT(d.head_block_time() >= HARDFORK_0_4_TIME, "Can only buyout after HARDFORK_0_4_TIME");

		auto post = d.get_post_by_platform(op.platform, op.poster, op.post_pid);// make sure pid exists
        FC_ASSERT((post.permission_flags & post_object::Post_Permission_Buyout) > 0, "post_object ${p} not allowed to buyout.", ("p", op.post_pid));
		post.receiptors_validate();
		auto iter = post.receiptors.find(op.receiptor_account_uid);
		FC_ASSERT(iter != post.receiptors.end(), 
                  "account ${a} isn`t a receiptor of the post ${p}", 
                  ("a", op.receiptor_account_uid)("p", op.post_pid));
        FC_ASSERT(iter->second.to_buyout && iter->second.buyout_ratio > 0 && iter->second.buyout_ratio <= iter->second.cur_ratio && iter->second.buyout_expiration >= d.head_block_time(),
			      "post ${p} `s receiptor`s buyout parameter is invalid. ${b}",
                  ("p", op.post_pid)("b", iter->second));
		if (iter->second.buyout_ratio < iter->second.cur_ratio)
		{
			if (post.receiptors.find(op.from_account_uid) == post.receiptors.end()) //add a new receiptor
			{
				FC_ASSERT(post.receiptors.size() < 5, "the num of post`s receiptors should be less than or equal to 5");
			}
		}

        const account_statistics_object* account_stats = &d.get_account_statistics_by_uid(op.from_account_uid);
        account_uid_type sign_account = sigs.real_secondary_uid(op.from_account_uid, 1);
        if (sign_account == op.platform)
        {
            auto auth_data = account_stats->prepaids_for_platform.find(sign_account);
            if (auth_data != account_stats->prepaids_for_platform.end())
            {
                sign_platform_uid = sign_account;
            }
        }

        auto auth_data = account_stats->prepaids_for_platform.find(op.platform);
        FC_ASSERT(auth_data != account_stats->prepaids_for_platform.end(), 
                  "platform ${p} not included in account ${a} `s prepaids_for_platform. ",
                  ("p", op.platform)("a", op.poster));
        FC_ASSERT((auth_data->second.permission_flags & account_statistics_object::Platform_Permission_Buyout) > 0, 
                  "the buyout permisson of platform ${p} authorized by account ${a} is invalid. ",
                  ("p", op.platform)("a", op.poster));
        FC_ASSERT(account_stats->prepaid >= iter->second.buyout_price, 
                  "Insufficient balance: unable to buyout, because the account ${a} `s prepaid [${c}] is less then needed [${n}]. ",
                  ("c", (account_stats->prepaid))("a", op.poster)("n", iter->second.buyout_price));
        if (auth_data->second.max_limit < GRAPHENE_MAX_PLATFORM_LIMIT_PREPAID && sign_platform_uid.valid())
        {
            share_type usable_prepaid = account_stats->get_auth_platform_usable_prepaid(*sign_platform_uid);
            FC_ASSERT(usable_prepaid >= iter->second.buyout_price,
                      "Insufficient balance: unable to buyout, because the prepaid [${c}] of platform ${p} authorized by account ${a} is less then needed [${n}]. ",
                      ("c", usable_prepaid)("p", *sign_platform_uid)("a", op.poster)("n", iter->second.buyout_price));
        }
            

		return void_result();
	}FC_CAPTURE_AND_RETHROW((op))
}

void_result buyout_evaluator::do_apply(const operation_type& op)
{
	try {
		database& d = db();
		const post_object& post = d.get_post_by_platform(op.platform, op.poster, op.post_pid);
		auto iter = post.receiptors.find(op.receiptor_account_uid);
		Recerptor_Parameter para = iter->second;
        d.modify(d.get_account_statistics_by_uid(op.from_account_uid), [&](account_statistics_object& obj)
        {
            if (sign_platform_uid.valid()) // signed by platform , then add auth cur_used
            {
                auto iter = obj.prepaids_for_platform.find(op.platform);
                iter->second.cur_used += para.buyout_price;
            }
            obj.prepaid -= para.buyout_price;
        });
        d.modify(d.get_account_statistics_by_uid(op.receiptor_account_uid), [&](account_statistics_object& obj)
        {
            obj.prepaid += para.buyout_price;
        });

		d.modify(post, [&](post_object& p) {
			if (para.buyout_ratio < para.cur_ratio)
			{
				auto old_receiptor = p.receiptors.find(op.receiptor_account_uid);
				old_receiptor->second.cur_ratio    = para.cur_ratio - para.buyout_ratio;
				old_receiptor->second.to_buyout    = false;
				old_receiptor->second.buyout_price = 0;
				old_receiptor->second.buyout_ratio = 0;
				p.receiptors.insert(make_pair(op.from_account_uid, Recerptor_Parameter{ para.buyout_ratio, false, 0, 0 }));
			}
			else if (para.buyout_ratio == para.cur_ratio)
			{
				p.receiptors.erase(op.receiptor_account_uid);
				p.receiptors.insert(make_pair(op.from_account_uid, Recerptor_Parameter{ para.buyout_ratio, false, 0, 0 }));
			}
		});

		return void_result();
	}FC_CAPTURE_AND_RETHROW((op))
}

void_result license_create_evaluator::do_evaluate(const operation_type& op)
{try {
    const database& d = db();
    FC_ASSERT(d.head_block_time() >= HARDFORK_0_4_TIME, "Can only create license after HARDFORK_0_4_TIME");

    d.get_platform_by_owner(op.platform); // make sure pid exists
    account_stats = &d.get_account_statistics_by_uid(op.platform);

    FC_ASSERT((account_stats->last_license_sequence + 1) == op.license_lid, "license id ${pid} is invalid.", ("pid", op.license_lid));

    const auto& licenses = d.get_index_type<license_index>().indices().get<by_license_lid>();
    auto itr = licenses.find(std::make_tuple(op.platform, op.license_lid));
    FC_ASSERT(itr == licenses.end(), "license ${license} already existed.", ("license", op.license_lid));

    return void_result();

}FC_CAPTURE_AND_RETHROW((op)) }

object_id_type license_create_evaluator::do_apply(const operation_type& op)
{try {
    database& d = db();

    d.modify(*account_stats, [&](account_statistics_object& s) {
        s.last_license_sequence += 1;
    });

    const auto& new_license_object = d.create<license_object>([&](license_object& obj)
    {
        obj.license_lid         = op.license_lid;
        obj.platform            = op.platform;
        obj.license_type        = op.type;
        obj.hash_value          = op.hash_value;
        obj.extra_data          = op.extra_data;
        obj.title               = op.title;
        obj.body                = op.body;

        obj.create_time         = d.head_block_time();
        obj.last_update_time    = d.head_block_time();
    });
    return new_license_object.id;

} FC_CAPTURE_AND_RETHROW((op)) }

} } // graphene::chain
