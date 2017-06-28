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
#include <graphene/chain/transfer_evaluator.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/is_authorized_asset.hpp>

namespace graphene { namespace chain {
void_result transfer_evaluator::do_evaluate( const transfer_operation& op )
{ try {
   
   const database& d = db();

   from_account    = &d.get_account_by_uid( op.from );
   to_account      = &d.get_account_by_uid( op.to );

   const asset_object&   transfer_asset_object      = asset_id_type(op.amount.asset_id)(d);

   try {

      GRAPHENE_ASSERT(
         is_authorized_asset( d, *from_account, transfer_asset_object ),
         transfer_from_account_not_whitelisted,
         "'from' account ${from} is not whitelisted for asset ${asset}.",
         ("from",op.from)
         ("asset",op.amount.asset_id)
         );
      GRAPHENE_ASSERT(
         is_authorized_asset( d, *to_account, transfer_asset_object ),
         transfer_to_account_not_whitelisted,
         "'to' account ${to} is not whitelisted for asset ${asset}.",
         ("to",op.to)
         ("asset",op.amount.asset_id)
         );

      if( transfer_asset_object.is_transfer_restricted() )
      {
         GRAPHENE_ASSERT(
            from_account->id == transfer_asset_object.issuer || to_account->id == transfer_asset_object.issuer,
            transfer_restricted_transfer_asset,
            "Asset {asset} has transfer_restricted flag enabled.",
            ("asset", op.amount.asset_id)
          );
      }

      // by default, from balance to balance
      asset_from_balance = asset_to_balance = op.amount;

      if ( op.extensions.valid() )
      {
         const auto& ev = op.extensions->value;
         if( ev.from_prepaid.valid() && ev.from_prepaid->amount > 0 )
         {
            asset_from_prepaid = *ev.from_prepaid;
            from_account_stats = &from_account->statistics(d);
            bool sufficient_prepaid = ( from_account_stats->prepaid >= asset_from_prepaid.amount );
            FC_ASSERT( sufficient_prepaid,
                       "Insufficient Prepaid: ${prepaid}, unable to transfer '${a}' from account '${f}' to '${t}'.",
                       ("f",from_account->uid)("t",to_account->uid)("a",d.to_pretty_string(asset_from_prepaid))
                       ("prepaid",d.to_pretty_core_string(from_account_stats->prepaid)) );
         }
         if( ev.from_balance.valid() )
            asset_from_balance = *ev.from_balance;
         else if( asset_from_prepaid.amount > 0 ) // if from_balance didn't present but from_prepaid presented
            asset_from_balance.amount = 0;

         if( ev.to_prepaid.valid() && ev.to_prepaid->amount > 0 )
         {
            asset_to_prepaid = *ev.to_prepaid;
            to_account_stats = &to_account->statistics(d);
         }
         if( ev.to_balance.valid() )
            asset_to_balance = *ev.to_balance;
         else if( asset_to_prepaid.amount > 0 ) // if to_balance didn't present but to_prepaid presented
            asset_to_balance.amount = 0;
      }

      if( asset_from_balance.amount > 0 )
      {
         const auto& from_balance = d.get_balance( *from_account, transfer_asset_object );
         bool sufficient_balance = ( from_balance.amount >= asset_from_balance.amount );
         FC_ASSERT( sufficient_balance,
                    "Insufficient Balance: ${balance}, unable to transfer '${a}' from account '${f}' to '${t}'.",
                    ("f",from_account->uid)("t",to_account->uid)("a",d.to_pretty_string(asset_from_balance))
                    ("balance",d.to_pretty_string(from_balance)) );
      }

      return void_result();
   } FC_RETHROW_EXCEPTIONS( error, "Unable to transfer ${a} from ${f} to ${t}", ("a",d.to_pretty_string(op.amount))("f",from_account->uid)("t",to_account->uid) );

}  FC_CAPTURE_AND_RETHROW( (op) ) }

void_result transfer_evaluator::do_apply( const transfer_operation& o )
{ try {
   database& d = db();
   if( asset_from_balance.amount > 0 )
      d.adjust_balance( *from_account, -asset_from_balance );
   if( asset_from_prepaid.amount > 0 )
      d.modify( *from_account_stats, [&](account_statistics_object& s)
      {
         s.prepaid -= asset_from_prepaid.amount;
      });
   if( asset_to_balance.amount > 0 )
      d.adjust_balance( *to_account, asset_to_balance );
   if( asset_to_prepaid.amount > 0 )
      d.modify( *to_account_stats, [&](account_statistics_object& s)
      {
         s.prepaid += asset_to_prepaid.amount;
      });
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }



void_result override_transfer_evaluator::do_evaluate( const override_transfer_operation& op )
{ try {
   const database& d = db();

   const asset_object&   asset_type      = asset_id_type(op.amount.asset_id)(d);
   GRAPHENE_ASSERT(
      asset_type.can_override(),
      override_transfer_not_permitted,
      "override_transfer not permitted for asset ${asset}",
      ("asset", op.amount.asset_id)
      );
   FC_ASSERT( asset_type.issuer == op.issuer );

   const account_object& from_account    = op.from(d);
   const account_object& to_account      = op.to(d);

   FC_ASSERT( is_authorized_asset( d, to_account, asset_type ) );
   FC_ASSERT( is_authorized_asset( d, from_account, asset_type ) );

   if( d.head_block_time() <= HARDFORK_419_TIME )
   {
      FC_ASSERT( is_authorized_asset( d, from_account, asset_type ) );
   }
   // the above becomes no-op after hardfork because this check will then be performed in evaluator

   FC_ASSERT( d.get_balance( from_account, asset_type ).amount >= op.amount.amount,
              "", ("total_transfer",op.amount)("balance",d.get_balance(from_account, asset_type).amount) );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result override_transfer_evaluator::do_apply( const override_transfer_operation& o )
{ try {
   // TODO review
   //db().adjust_balance( o.from, -o.amount );
   //db().adjust_balance( o.to, o.amount );
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

} } // graphene::chain
