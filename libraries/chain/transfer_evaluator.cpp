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

   const asset_object&   transfer_asset_object      = d.get_asset_by_aid( op.amount.asset_id );

   validate_authorized_asset( d, *from_account, transfer_asset_object, "'from' " );
   validate_authorized_asset( d, *to_account,   transfer_asset_object, "'to' " );

   if (!op.some_from_balance())
   {
       account_uid_type sign_account = sigs.real_secondary_uid(op.from, 1);
       if (sign_account != op.from)
       {
           const auto& account_stats = d.get_account_statistics_by_uid(op.from);
           auth_object = d.find_account_auth_platform_object_by_account_platform(op.from, sign_account);
           if (auth_object){   //transfer by platform,
               FC_ASSERT(auth_object->is_active == true, "account_auth_platform_object is not active. ");
               FC_ASSERT((auth_object->permission_flags & account_auth_platform_object::Platform_Permission_Transfer) > 0,
                   "the transfer permisson of platform ${p} authorized by account ${a} is invalid. ",
                   ("a", (op.from))("p", sign_account));
               FC_ASSERT(account_stats.prepaid >= op.amount.amount,
                   "Insufficient balance: unable to transfer, because the account ${a} `s prepaid [${c}] is less than needed [${n}]. ",
                   ("c", (account_stats.prepaid))("a", op.from)("n", op.amount.amount));
               if (auth_object->max_limit < GRAPHENE_MAX_PLATFORM_LIMIT_PREPAID)
               {
                   share_type usable_prepaid = auth_object->get_auth_platform_usable_prepaid(account_stats.prepaid);
                   FC_ASSERT(usable_prepaid >= op.amount.amount,
                       "Insufficient balance: unable to forward, because the prepaid [${c}] of platform ${p} authorized by account ${a} is less than needed [${n}]. ",
                       ("c", usable_prepaid)("p", sign_account)("a", op.from)("n", op.amount.amount));
               }
           }
       }
   }

   try {

      if( transfer_asset_object.is_transfer_restricted() )
      {
         GRAPHENE_ASSERT(
            from_account->uid == transfer_asset_object.issuer || to_account->uid == transfer_asset_object.issuer,
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
   if (asset_from_prepaid.amount > 0){
       d.modify(*from_account_stats, [&](account_statistics_object& s)
       {
           s.prepaid -= asset_from_prepaid.amount;
       });

       if (auth_object)
       {
           d.modify(*auth_object, [&](account_auth_platform_object& obj)
           {
               obj.cur_used += asset_from_prepaid.amount;
           });
       }
   }
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

   const asset_object& asset_type = d.get_asset_by_aid( op.amount.asset_id );
   GRAPHENE_ASSERT(
      asset_type.can_override(),
      override_transfer_not_permitted,
      "override_transfer not permitted for asset ${asset}",
      ("asset", op.amount.asset_id)
      );
   FC_ASSERT( asset_type.issuer == op.issuer, "only asset issuer can override-transfer asset" );

   const account_object& from_account    = d.get_account_by_uid( op.from );
   const account_object& to_account      = d.get_account_by_uid( op.to );

   // only check 'to', because issuer should always be able to override-transfer from any account
   validate_authorized_asset( d, to_account, asset_type, "'to' " );

   FC_ASSERT( d.get_balance( from_account, asset_type ).amount >= op.amount.amount,
              "", ("total_transfer",op.amount)("balance",d.get_balance(from_account, asset_type).amount) );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result override_transfer_evaluator::do_apply( const override_transfer_operation& o )
{ try {
   db().adjust_balance( o.from, -o.amount );
   db().adjust_balance( o.to, o.amount );
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

} } // graphene::chain
