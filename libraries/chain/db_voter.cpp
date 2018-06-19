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
#include <graphene/chain/db_with.hpp>

#include <graphene/chain/global_property_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/content_object.hpp>

#include <fc/uint128.hpp>

namespace graphene { namespace chain {

void database::update_voter_effective_votes( const voter_object& voter )
{
   const auto& global_params = get_global_properties().parameters;
   const auto window = global_params.max_governance_votes_seconds;
   const auto now = head_block_time();

   // update effective_votes
   const auto old_votes = voter.effective_votes;
   if( voter.effective_votes == voter.votes )
   {
      modify( voter, [&]( voter_object& v )
      {
         v.effective_votes_last_update = now;
         v.effective_votes_next_update_block = -1;
      } );
   }
   else if( voter.effective_votes > voter.votes || now >= voter.votes_last_update + window )
   {
      modify( voter, [&]( voter_object& v )
      {
         v.effective_votes = v.votes;
         v.effective_votes_last_update = now;
         v.effective_votes_next_update_block = -1;
      } );
   }
   else if( now > voter.effective_votes_last_update )
   {
      // need to schedule next update because effective_votes < votes, and need to update effective_votes
      uint64_t delta_seconds = ( now - voter.effective_votes_last_update ).to_seconds();
      uint64_t old_seconds = window - delta_seconds;

      fc::uint128_t old_coin_seconds = fc::uint128_t( voter.effective_votes ) * old_seconds;
      fc::uint128_t new_coin_seconds = fc::uint128_t( voter.votes ) * delta_seconds;

      uint64_t new_average_coins = ( ( old_coin_seconds + new_coin_seconds ) / window ).to_uint64();

      modify( voter, [&]( voter_object& v )
      {
         v.effective_votes = new_average_coins;
         v.effective_votes_last_update = now;
         v.effective_votes_next_update_block = head_block_num() + global_params.governance_votes_update_interval;
      } );
   }
   else
   {
      // need to schedule next update because effective_votes < votes, but no need to update effective_votes
      modify( voter, [&]( voter_object& v )
      {
         v.effective_votes_next_update_block = head_block_num() + global_params.governance_votes_update_interval;
      } );
   }

   if( old_votes != voter.effective_votes )
   {
      share_type delta = voter.effective_votes;
      delta -= old_votes;
      adjust_voter_votes( voter, delta );
   }

}

void database::adjust_voter_votes( const voter_object& voter, share_type delta )
{
   const auto max_level = get_global_properties().parameters.max_governance_voting_proxy_level;

   const voter_object* current_voter = &voter;
   uint8_t level = 0;
   while( current_voter->proxy_uid != GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID && level < max_level )
   {
      current_voter = find_voter( current_voter->proxy_uid, current_voter->proxy_sequence );
      modify( *current_voter, [&]( voter_object& v )
      {
         v.proxied_votes[level] += delta.value;
      } );
      ++level;
   }

   if( current_voter->proxy_uid == GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID )
   {
      adjust_voter_self_votes( *current_voter, delta );
   }

}

void database::adjust_voter_self_votes( const voter_object& voter, share_type delta )
{
   adjust_voter_self_witness_votes( voter, delta );
   adjust_voter_self_committee_member_votes( voter, delta );
   adjust_voter_self_platform_votes( voter, delta );
}

void database::adjust_voter_self_witness_votes( const voter_object& voter, share_type delta )
{
   // adjust witness votes
   uint16_t invalid_witness_votes_removed = 0;
   const auto& idx = get_index_type<witness_vote_index>().indices().get<by_voter_seq>();
   auto itr = idx.lower_bound( std::make_tuple( voter.uid, voter.sequence ) );
   while( itr != idx.end() && itr->voter_uid == voter.uid && itr->voter_sequence == voter.sequence )
   {
      const witness_object* witness = find_witness_by_uid( itr->witness_uid );
      bool to_remove = false;
      if( witness != nullptr && witness->sequence == itr->witness_sequence )
         adjust_witness_votes( *witness, delta );
      else
      {
         to_remove = true;
         invalid_witness_votes_removed += 1;
      }
      auto old_itr = itr;
      ++itr;
      if( to_remove )
         remove( *old_itr );
   }
   if( invalid_witness_votes_removed > 0 )
   {
      modify( voter, [&]( voter_object& v )
      {
         v.number_of_witnesses_voted -= invalid_witness_votes_removed;
      } );
   }
}

void database::adjust_voter_self_platform_votes( const voter_object& voter, share_type delta )
{
   // adjust platform votes
   uint16_t invalid_platform_votes_removed = 0;
   const auto& idx = get_index_type<platform_vote_index>().indices().get<by_platform_voter_seq>();
   auto itr = idx.lower_bound( std::make_tuple( voter.uid, voter.sequence ) );
   while( itr != idx.end() && itr->voter_uid == voter.uid && itr->voter_sequence == voter.sequence )
   {
      const platform_object* pla = find_platform_by_owner( itr->platform_owner );
      bool to_remove = false;
      if( pla != nullptr && pla->sequence == itr->platform_sequence )
         adjust_platform_votes( *pla, delta );
      else
      {
         to_remove = true;
         invalid_platform_votes_removed += 1;
      }
      auto old_itr = itr;
      ++itr;
      if( to_remove )
         remove( *old_itr );
   }
   if( invalid_platform_votes_removed > 0 )
   {
      modify( voter, [&]( voter_object& v )
      {
         v.number_of_platform_voted -= invalid_platform_votes_removed;
      } );
   }
}

void database::adjust_voter_self_committee_member_votes( const voter_object& voter, share_type delta )
{
   // adjust committee_member votes
   uint16_t invalid_committee_member_votes_removed = 0;
   const auto& idx = get_index_type<committee_member_vote_index>().indices().get<by_voter_seq>();
   auto itr = idx.lower_bound( std::make_tuple( voter.uid, voter.sequence ) );
   while( itr != idx.end() && itr->voter_uid == voter.uid && itr->voter_sequence == voter.sequence )
   {
      const committee_member_object* committee_member = find_committee_member_by_uid( itr->committee_member_uid );
      bool to_remove = false;
      if( committee_member != nullptr && committee_member->sequence == itr->committee_member_sequence )
         adjust_committee_member_votes( *committee_member, delta );
      else
      {
         to_remove = true;
         invalid_committee_member_votes_removed += 1;
      }
      auto old_itr = itr;
      ++itr;
      if( to_remove )
         remove( *old_itr );
   }
   if( invalid_committee_member_votes_removed > 0 )
   {
      modify( voter, [&]( voter_object& v )
      {
         v.number_of_committee_members_voted -= invalid_committee_member_votes_removed;
      } );
   }
   // TODO adjust committee votes
}

void database::adjust_voter_proxy_votes( const voter_object& voter, vector<share_type> delta, bool update_last_vote )
{
   const auto max_level = get_global_properties().parameters.max_governance_voting_proxy_level;

   const voter_object* current_voter = &voter;
   uint8_t level = 0;
   vector<const voter_object*> vec;
   if( update_last_vote )
      vec.push_back( current_voter );
   while( level < max_level )
   {
      current_voter = find_voter( current_voter->proxy_uid, current_voter->proxy_sequence );
      if( update_last_vote )
         vec.push_back( current_voter );
      modify( *current_voter, [&]( voter_object& v )
      {
         for( uint8_t j = level; j < max_level; ++j )
            v.proxied_votes[j] += delta[j-level].value;
      } );
      if( current_voter->proxy_uid == GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID )
         break;
      ++level;
   }

   if( update_last_vote )
   {
      for( uint8_t i = vec.size() - 1; i > 0; --i )
      {
         modify( *vec[i-1], [&]( voter_object& v )
         {
            for( uint8_t j = 1; j <= max_level; ++j )
            {
               v.proxy_last_vote_block[ j ] = vec[i]->proxy_last_vote_block[ j-1 ];
            }
            v.update_effective_last_vote_block();
         } );
      }
   }

   if( current_voter->proxy_uid == GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID )
   {
      share_type total_delta = 0;
      for( uint8_t j = level; j < max_level; ++j )
      {
         total_delta += delta[j-level];
      }
      adjust_voter_self_votes( *current_voter, total_delta );
   }
}

void database::clear_voter_witness_votes( const voter_object& voter )
{
   const share_type votes = voter.total_votes();
   const auto& idx = get_index_type<witness_vote_index>().indices().get<by_voter_seq>();
   auto itr = idx.lower_bound( std::make_tuple( voter.uid, voter.sequence ) );
   while( itr != idx.end() && itr->voter_uid == voter.uid && itr->voter_sequence == voter.sequence )
   {
      const witness_object* witness = find_witness_by_uid( itr->witness_uid );
      if( witness != nullptr && witness->sequence == itr->witness_sequence )
      {
         adjust_witness_votes( *witness, -votes );
      }
      auto old_itr = itr;
      ++itr;
      remove( *old_itr );
   }
   modify( voter, [&](voter_object& v) {
      v.number_of_witnesses_voted = 0;
   });
}

void database::clear_voter_platform_votes( const voter_object& voter )
{
   const share_type votes = voter.total_votes();
   const auto& idx = get_index_type<platform_vote_index>().indices().get<by_platform_voter_seq>();
   auto itr = idx.lower_bound( std::make_tuple( voter.uid, voter.sequence ) );
   while( itr != idx.end() && itr->voter_uid == voter.uid && itr->voter_sequence == voter.sequence )
   {
      const platform_object* pla = find_platform_by_owner( itr->platform_owner );
      if( pla != nullptr && pla->sequence == itr->platform_sequence )
      {
         adjust_platform_votes( *pla, -votes );
      }
      auto old_itr = itr;
      ++itr;
      remove( *old_itr );
   }
   modify( voter, [&](voter_object& v) {
      v.number_of_platform_voted = 0;
   });
}

void database::clear_voter_committee_member_votes( const voter_object& voter )
{
   const share_type votes = voter.total_votes();
   const auto& idx = get_index_type<committee_member_vote_index>().indices().get<by_voter_seq>();
   auto itr = idx.lower_bound( std::make_tuple( voter.uid, voter.sequence ) );
   while( itr != idx.end() && itr->voter_uid == voter.uid && itr->voter_sequence == voter.sequence )
   {
      const committee_member_object* committee_member = find_committee_member_by_uid( itr->committee_member_uid );
      if( committee_member != nullptr && committee_member->sequence == itr->committee_member_sequence )
      {
         adjust_committee_member_votes( *committee_member, -votes );
      }
      auto old_itr = itr;
      ++itr;
      remove( *old_itr );
   }
   modify( voter, [&](voter_object& v) {
      v.number_of_committee_members_voted = 0;
   });
}

void database::clear_voter_proxy_votes( const voter_object& voter )
{
   FC_ASSERT( voter.proxy_uid != GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID );

   const auto max_level = get_global_properties().parameters.max_governance_voting_proxy_level;

   vector<share_type> delta( max_level ); // [ -self, -proxied_level1, -proxied_level2, ... ]
   delta[0] = -share_type( voter.effective_votes );
   for( uint8_t i = 1; i < max_level; ++i )
   {
      delta[i] = -share_type( voter.proxied_votes[i-1] );
   }

   adjust_voter_proxy_votes( voter, delta, true );
}

void database::clear_voter_votes( const voter_object& voter )
{
   if( voter.proxy_uid == GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID ) // voting by self
   {
      // remove its all witness votes
      clear_voter_witness_votes( voter );
      // remove its all committee votes
      clear_voter_committee_member_votes( voter );
      // remove its all platform votes
      clear_voter_platform_votes( voter );
   }
   else // voting with a proxy
   {
      clear_voter_proxy_votes( voter );
   }

}

void database::invalidate_voter( const voter_object& voter )
{
   if( !voter.is_valid )
      return;

   clear_voter_votes( voter );

   // update proxy voter
   if( voter.proxy_uid != GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID )
   {
      const voter_object* proxy_voter = find_voter( voter.proxy_uid, voter.proxy_sequence );
      modify( *proxy_voter, [&](voter_object& v) {
         v.proxied_voters -= 1;
      });
   }

   // update account statistics
   modify( get_account_statistics_by_uid( voter.uid ), [&](account_statistics_object& s) {
      s.is_voter = false;
   });

   // update voter info
   modify( voter, [&](voter_object& v) {
      v.is_valid = false;
      // begin
      // not really need to update these fields, since they will not be used before the object get removed.
      v.votes = 0;
      v.votes_last_update = head_block_time();
      v.effective_votes = 0;
      v.effective_votes_last_update = head_block_time();
      // end
      v.effective_votes_next_update_block = -1; // to avoid scheduled updating
      if( voter.proxy_uid != GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID )
      {
         v.proxy_uid = GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID;
         v.proxy_sequence = 0;
      }
   });
}

bool database::check_voter_valid( const voter_object& voter, bool deep_check )
{
   if( !deep_check )
      return voter.is_valid;

   if( voter.is_valid == false )
      return false;

   const auto& global_params = get_global_properties().parameters;
   const uint32_t expire_blocks = global_params.governance_voting_expiration_blocks;
   const uint32_t head_num = head_block_num();
   if( head_num < expire_blocks )
      return true;

   const uint32_t max_last_vote_block = head_num - expire_blocks;

   const voter_object* current_voter = &voter;
   const uint8_t max_level = global_params.max_governance_voting_proxy_level;
   for( uint8_t level = max_level; ; --level )
   {
      for( uint8_t i = 0; i <= level; ++i )
      {
         if( current_voter->proxy_last_vote_block[i] > max_last_vote_block )
            return true;
      }
      if( current_voter->proxy_uid == GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID || level == 0 )
         return false;
      else
      {
         current_voter = find_voter( current_voter->proxy_uid, current_voter->proxy_sequence );
         if( current_voter == nullptr )
            return false;
      }
   }
   return false;
}

uint32_t database::process_invalid_proxied_voters( const voter_object& proxy, uint32_t max_voters_to_process )
{
   if( max_voters_to_process == 0 )
      return 0;

   FC_ASSERT( !proxy.is_valid, "This function should only be called with an invalid proxy" );

   const auto max_level = get_global_properties().parameters.max_governance_voting_proxy_level;
   const auto expire_blocks = get_global_properties().parameters.governance_voting_expiration_blocks;
   const auto head_num = head_block_num();

   uint32_t processed = 0;
   const auto& idx = get_index_type<voter_index>().indices().get<by_proxy>();
   auto itr = idx.lower_bound( std::make_tuple( proxy.uid, proxy.sequence ) );
   while( processed < max_voters_to_process
         && itr != idx.end() && itr->proxy_uid == proxy.uid && itr->proxy_sequence == proxy.sequence )
   {
      ++processed;
      bool was_valid = itr->is_valid;
      // need to keep track of proxy_last_vote_block so voters who proxied to this can be updated correctly
      // Note: after this, proxy's `proxied_votes` would become stale, but it doesn't matter,
      //       because it will not be used anymore and the object will be removed soon
      modify( *itr, [&](voter_object& v) {
         // update proxy_last_vote_block, effective_last_vote_block
         for( uint8_t i = 1; i <= max_level; ++i )
         {
            v.proxy_last_vote_block[ i ] = proxy.proxy_last_vote_block[ i-1 ];
         }
         v.update_effective_last_vote_block();
         // check if it's still valid
         if( v.is_valid && v.effective_last_vote_block + expire_blocks <= head_num )
         {
            v.is_valid = false;
            // begin
            // not really need to update these fields, since they will not be used before the object get removed.
            v.votes = 0;
            v.votes_last_update = head_block_time();
            v.effective_votes = 0;
            v.effective_votes_last_update = head_block_time();
            // end
            v.effective_votes_next_update_block = -1; // to avoid scheduled updating
         }
         // the proxy is invalid, so should change this voter's proxy to self
         v.proxy_uid = GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID;
         v.proxy_sequence = 0;
      });

      if( was_valid && itr->is_valid == false )
      {
         // update account statistics
         modify( get_account_statistics_by_uid( itr->uid ), [&](account_statistics_object& s) {
            s.is_voter = false;
         });
      }
      // else do nothing

      ++itr;
   }

   if( processed > 0 )
   {
      modify( proxy, [&](voter_object& v) {
         v.proxied_voters -= processed;
      });
   }

   if( proxy.proxied_voters == 0 )
      remove( proxy );

   return processed;
}

void database::adjust_committee_member_votes( const committee_member_object& committee_member, share_type delta )
{
   if( delta == 0 || !committee_member.is_valid )
      return;

   modify( committee_member, [&]( committee_member_object& w )
   {
      w.total_votes += delta.value;
   } );
}

} }
