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
#include <graphene/chain/protocol/committee_member.hpp>

namespace graphene { namespace chain {

void committee_member_create_operation::validate()const
{
   validate_op_fee( fee, "committee member creation " );
   validate_account_uid( account, "committee member " );
   validate_non_negative_asset( pledge, "pledge" );
   FC_ASSERT( url.size() < GRAPHENE_MAX_URL_LENGTH, "url is too long" );
}

void committee_member_update_operation::validate()const
{
   validate_op_fee( fee, "committee member update " );
   validate_account_uid( account, "committee member " );
   FC_ASSERT( new_pledge.valid() || new_url.valid(), "Should change something" );
   if( new_pledge.valid() )
      validate_non_negative_asset( *new_pledge, "new pledge" );
   if( new_url.valid() )
      FC_ASSERT( new_url->size() < GRAPHENE_MAX_URL_LENGTH, "new url is too long" );
}

void committee_member_vote_update_operation::validate() const
{
   validate_op_fee( fee, "committee member vote update " );
   validate_account_uid( voter, "voter " );
   auto itr_add = committee_members_to_add.begin();
   auto itr_remove = committee_members_to_remove.begin();
   while( itr_add != committee_members_to_add.end() && itr_remove != committee_members_to_remove.end() )
   {
      FC_ASSERT( *itr_add != *itr_remove, "Can not add and remove same committee member, uid: ${w}", ("w",*itr_add) );
      if( *itr_add < *itr_remove )
         ++itr_add;
      else
         ++itr_remove;
   }
   for( const auto uid : committee_members_to_add )
      validate_account_uid( uid, "committee member " );
   for( const auto uid : committee_members_to_remove )
      validate_account_uid( uid, "committee member " );
   // can change nothing to only refresh last_vote_update_time
}

void committee_member_update_global_parameters_operation::validate() const
{
   FC_ASSERT( fee.amount >= 0 );
   new_parameters.validate();
}

} } // graphene::chain
