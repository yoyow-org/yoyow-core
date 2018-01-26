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
#include <graphene/chain/protocol/content.hpp>

namespace graphene { namespace chain {

void platform_create_operation::validate() const
{
   validate_op_fee( fee, "platform creation " );
   validate_account_uid( account, "platform " );
   validate_non_negative_asset( pledge, "pledge" );
   FC_ASSERT( url.size() < GRAPHENE_MAX_URL_LENGTH, "url is too long" );
   FC_ASSERT( name.size() < GRAPHENE_MAX_PLATFORM_NAME_LENGTH, "name is too long" );
   FC_ASSERT( extra_data.size() < GRAPHENE_MAX_PLATFORM_EXTRA_DATA_LENGTH, "extra_data is too long" );
}

share_type platform_create_operation::calculate_fee(const fee_parameters_type& k)const
{
   share_type core_fee_required = k.fee;
   core_fee_required += calculate_data_fee( fc::raw::pack_size(extra_data), k.price_per_kbyte );
   core_fee_required += calculate_data_fee( fc::raw::pack_size(name), k.price_per_kbyte );
   core_fee_required += calculate_data_fee( fc::raw::pack_size(url), k.price_per_kbyte );
   return core_fee_required;
}

void platform_update_operation::validate() const
{
   validate_op_fee( fee, "platform update " );
   validate_account_uid( account, "platform " );
   FC_ASSERT( new_pledge.valid() || new_name.valid() || new_url.valid() || new_extra_data.valid(), "Should change something" );
   if( new_pledge.valid() )
      validate_non_negative_asset( *new_pledge, "new pledge" );
   if( new_url.valid() )
      FC_ASSERT( new_url->size() < GRAPHENE_MAX_URL_LENGTH, "new url is too long" );
   if( new_name.valid() )
      FC_ASSERT( new_name->size() < GRAPHENE_MAX_PLATFORM_NAME_LENGTH, "new name is too long" );
   if( new_extra_data.valid() )
      FC_ASSERT( new_extra_data->size() < GRAPHENE_MAX_PLATFORM_EXTRA_DATA_LENGTH, "new extra_data is too long" );
}

share_type platform_update_operation::calculate_fee(const fee_parameters_type& k)const
{
   share_type core_fee_required = k.fee;
   if( new_extra_data.valid() )
      core_fee_required += calculate_data_fee( fc::raw::pack_size(new_extra_data), k.price_per_kbyte );
   if( new_name.valid() )
      core_fee_required += calculate_data_fee( fc::raw::pack_size(new_name), k.price_per_kbyte );
   if( new_url.valid() )
      core_fee_required += calculate_data_fee( fc::raw::pack_size(new_url), k.price_per_kbyte );
   return core_fee_required;
}

void platform_vote_update_operation::validate()const
{
   validate_op_fee( fee, "platform vote update " );
   validate_account_uid( voter, "voter " );
   auto itr_add = platform_to_add.begin();
   auto itr_remove = platform_to_remove.begin();
   while( itr_add != platform_to_add.end() && itr_remove != platform_to_remove.end() )
   {
      FC_ASSERT( *itr_add != *itr_remove, "Can not add and remove same platform, uid: ${w}", ("w",*itr_add) );
      if( *itr_add < *itr_remove )
         ++itr_add;
      else
         ++itr_remove;
   }
   for( const auto uid : platform_to_add )
      validate_account_uid( uid, "platform " );
   for( const auto uid : platform_to_remove )
      validate_account_uid( uid, "platform " );
}

share_type platform_vote_update_operation::calculate_fee(const fee_parameters_type& k)const
{
   auto core_fee_required = k.basic_fee;

   auto total_size = platform_to_add.size();
   core_fee_required += ( k.price_per_platform * total_size );

   return core_fee_required;
}

share_type post_operation::calculate_fee( const fee_parameters_type& schedule )const
{
   share_type core_fee_required = schedule.fee;
   core_fee_required += calculate_data_fee( fc::raw::pack_size(extra_data), schedule.price_per_kbyte );
   core_fee_required += calculate_data_fee( fc::raw::pack_size(title), schedule.price_per_kbyte );
   core_fee_required += calculate_data_fee( fc::raw::pack_size(body), schedule.price_per_kbyte );
   return core_fee_required;
}


void post_operation::validate()const
{
   validate_op_fee( fee, "post " );
   validate_account_uid( poster, "poster " );
   FC_ASSERT( parent_poster.valid() == parent_post_pid.valid(), "parent poster and pid should be both presented or both not" );
   if( parent_poster.valid() )
      validate_account_uid( *parent_poster, "parent poster " );
}



} } // graphene::chain
