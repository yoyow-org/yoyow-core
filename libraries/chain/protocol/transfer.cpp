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
#include <graphene/chain/protocol/transfer.hpp>

namespace graphene { namespace chain {

share_type transfer_operation::calculate_fee( const fee_parameters_type& schedule )const
{
   share_type core_fee_required = schedule.fee;
   if( memo )
      core_fee_required += calculate_data_fee( fc::raw::pack_size(memo), schedule.price_per_kbyte );
   return core_fee_required;
}


void transfer_operation::validate()const
{
   validate_op_fee( fee, "transfer " );
   validate_account_uid( from, "from " );
   validate_account_uid( to, "to " );
   validate_positive_asset( amount, "transfer amount" );
   if ( !extensions.valid() || amount.asset_id > GRAPHENE_CORE_ASSET_AID )  //Non-core asset and no extensions
   {
      FC_ASSERT( from != to, "can not transfer to self." );
   }
   else
   {
      const auto& ev = extensions->value;
      asset from_balance, from_prepaid, to_balance, to_prepaid;
      if( ev.from_balance.valid() )
      {
         from_balance = *ev.from_balance;
         validate_positive_asset( from_balance, "transfer from_balance" );
      }
      if( ev.from_prepaid.valid() )
      {
         from_prepaid = *ev.from_prepaid;
         validate_positive_asset( from_prepaid, "transfer from_prepaid" );
      }
      if( ev.to_balance.valid() )
      {
         to_balance = *ev.to_balance;
         validate_positive_asset( to_balance, "transfer to_balance" );
      }
      if( ev.to_prepaid.valid() )
      {
         to_prepaid = *ev.to_prepaid;
         validate_positive_asset( to_prepaid, "transfer to_prepaid" );
      }
      if( from_balance.amount > 0 || from_prepaid.amount > 0 )
      {
         FC_ASSERT( amount == from_balance + from_prepaid, "amount should be equal to sum of from_balance and from_prepaid." );
      }
      if( to_balance.amount > 0 || to_prepaid.amount > 0 )
      {
         FC_ASSERT( amount == to_balance + to_prepaid, "amount should be equal to sum of to_balance and to_prepaid." );
      }
      if( from == to )
      {
         bool only_from_balance_to_prepaid = ( from_prepaid.amount == 0 && to_prepaid.amount == amount.amount );
         bool only_from_prepaid_to_balance = ( from_prepaid.amount == amount.amount && to_prepaid.amount == 0 );
         FC_ASSERT( only_from_balance_to_prepaid || only_from_prepaid_to_balance,
                    "when tranferring to self, can only transfer from balance to prepaid, or from prepaid to balance" );
      }
   }
}

share_type override_transfer_operation::calculate_fee( const fee_parameters_type& schedule )const
{
   share_type core_fee_required = schedule.fee;
   if( memo )
      core_fee_required += calculate_data_fee( fc::raw::pack_size(memo), schedule.price_per_kbyte );
   return core_fee_required;
}


void override_transfer_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( from != to );
   FC_ASSERT( amount.amount > 0 );
   FC_ASSERT( issuer != from );
}

} } // graphene::chain
