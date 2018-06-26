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
#include <graphene/chain/protocol/witness.hpp>

namespace graphene { namespace chain {

void witness_create_operation::validate() const
{
   validate_op_fee( fee, "witness creation " );
   validate_account_uid( account, "witness " );
   validate_non_negative_core_asset( pledge, "pledge" );
   FC_ASSERT( url.size() < GRAPHENE_MAX_URL_LENGTH, "url is too long" );
}

void witness_update_operation::validate() const
{
   validate_op_fee( fee, "witness update " );
   validate_account_uid( account, "witness " );
   FC_ASSERT( new_pledge.valid() || new_signing_key.valid() || new_url.valid(), "Should change something" );
   if( new_pledge.valid() )
      validate_non_negative_core_asset( *new_pledge, "new pledge" );
   if( new_url.valid() )
      FC_ASSERT( new_url->size() < GRAPHENE_MAX_URL_LENGTH, "new url is too long" );
}

share_type witness_vote_update_operation::calculate_fee( const fee_parameters_type& k )const
{
   share_type core_fee_required = k.basic_fee;

   auto total_size = witnesses_to_add.size();// + witnesses_to_remove.size();
   core_fee_required += ( share_type( k.price_per_witness ) * total_size );

   return core_fee_required;
}

void witness_vote_update_operation::validate() const
{
   validate_op_fee( fee, "witness vote update " );
   validate_account_uid( voter, "voter " );
   auto itr_add = witnesses_to_add.begin();
   auto itr_remove = witnesses_to_remove.begin();
   while( itr_add != witnesses_to_add.end() && itr_remove != witnesses_to_remove.end() )
   {
      FC_ASSERT( *itr_add != *itr_remove, "Can not add and remove same witness, uid: ${w}", ("w",*itr_add) );
      if( *itr_add < *itr_remove )
         ++itr_add;
      else
         ++itr_remove;
   }
   for( const auto uid : witnesses_to_add )
      validate_account_uid( uid, "witness " );
   for( const auto uid : witnesses_to_remove )
      validate_account_uid( uid, "witness " );
   // can change nothing to only refresh last_vote_update_time
}

void witness_collect_pay_operation::validate() const
{
   validate_op_fee( fee, "witness pay collecting " );
   validate_account_uid( account, "witness " );
   validate_positive_core_asset( pay, "pay" );
}

void witness_report_operation::validate() const
{
   validate_op_fee( fee, "witness report " );
   validate_account_uid( reporter, "reporting " );
   validate_account_uid( first_block.witness, "reported witness " );
   FC_ASSERT( first_block.timestamp == second_block.timestamp, "first block and second block should have same timestamp" );
   FC_ASSERT( first_block.witness == second_block.witness, "first block and second block should be produced by same witness" );
   FC_ASSERT( first_block.id() != second_block.id(), "first block and second block should be different" );
   FC_ASSERT( first_block.block_num() > 0, "reporting block number should be positive" );
   FC_ASSERT( first_block.block_num() == second_block.block_num(), "first block and second block should have same block number" );
   FC_ASSERT( first_block.signee() == second_block.signee(), "first block and second block should be signed by same key" );
}

} } // graphene::chain
