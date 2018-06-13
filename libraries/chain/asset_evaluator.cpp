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
#include <graphene/chain/asset_evaluator.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/is_authorized_asset.hpp>

#include <functional>

namespace graphene { namespace chain {

void_result asset_create_evaluator::do_evaluate( const asset_create_operation& op )
{ try {
   database& d = db();
   FC_ASSERT( d.head_block_time() >= HARDFORK_0_3_TIME, "Can only be asset_create after HARDFORK_0_3_TIME" );

   const auto& chain_parameters = d.get_global_properties().parameters;
   FC_ASSERT( op.common_options.whitelist_authorities.size() <= chain_parameters.maximum_asset_whitelist_authorities );
   FC_ASSERT( op.common_options.blacklist_authorities.size() <= chain_parameters.maximum_asset_whitelist_authorities );

   // Check that all authorities do exist
   for( auto id : op.common_options.whitelist_authorities )
      d.get_account_by_uid(id);
   for( auto id : op.common_options.blacklist_authorities )
      d.get_account_by_uid(id);

   auto& asset_indx = d.get_index_type<asset_index>().indices().get<by_symbol>();
   auto asset_symbol_itr = asset_indx.find( op.symbol );
   FC_ASSERT( asset_symbol_itr == asset_indx.end() );

      auto dotpos = op.symbol.rfind( '.' );
      if( dotpos != std::string::npos )
      {
            auto prefix = op.symbol.substr( 0, dotpos );
            auto asset_symbol_itr = asset_indx.find( prefix );
            FC_ASSERT( asset_symbol_itr != asset_indx.end(), "Asset ${s} may only be created by issuer of ${p}, but ${p} has not been registered",
                        ("s",op.symbol)("p",prefix) );
            FC_ASSERT( asset_symbol_itr->issuer == op.issuer, "Asset ${s} may only be created by issuer of ${p}, ${i}",
                        ("s",op.symbol)("p",prefix)("i", d.get_account_by_uid( op.issuer ).name) );
      }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type asset_create_evaluator::do_apply( const asset_create_operation& op )
{ try {
   database& d = db();

   auto next_asset_id = d.get_index_type<asset_index>().get_next_id();

   share_type initial_supply = 0;
   if( op.extensions.valid() && op.extensions->value.initial_supply.valid() )
      initial_supply = *op.extensions->value.initial_supply;

   const asset_dynamic_data_object& dyn_asset =
      d.create<asset_dynamic_data_object>( [next_asset_id,initial_supply]( asset_dynamic_data_object& a ) {
         a.asset_id = next_asset_id.instance();
         a.current_supply = initial_supply;
      });

   const asset_object& new_asset =
     d.create<asset_object>( [&op,next_asset_id,&dyn_asset]( asset_object& a ) {
         a.issuer = op.issuer;
         a.symbol = op.symbol;
         a.precision = op.precision;
         a.options = op.common_options;
         a.asset_id = next_asset_id.instance();
         a.dynamic_asset_data_id = dyn_asset.id;
      });

   FC_ASSERT( new_asset.id == next_asset_id );

   if( initial_supply > 0 )
      d.adjust_balance( op.issuer, new_asset.amount( initial_supply ) );

   return new_asset.id;
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result asset_issue_evaluator::do_evaluate( const asset_issue_operation& o )
{ try {
   const database& d = db();
   FC_ASSERT( d.head_block_time() >= HARDFORK_0_3_TIME, "Can only be asset_issue after HARDFORK_0_3_TIME" );

   const asset_object& a = d.get_asset_by_aid( o.asset_to_issue.asset_id );
   FC_ASSERT( o.issuer == a.issuer, "only asset issuer can issue asset" );

   FC_ASSERT( a.can_issue_asset(), "'issue_asset' flag is disabled for this asset" );

   to_account = &d.get_account_by_uid( o.issue_to_account );
   FC_ASSERT( is_authorized_asset( d, *to_account, a ) );

   asset_dyn_data = &a.dynamic_asset_data_id(d);
   FC_ASSERT( (asset_dyn_data->current_supply + o.asset_to_issue.amount) <= a.options.max_supply,
              "can not create more than max supply" );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_issue_evaluator::do_apply( const asset_issue_operation& o )
{ try {
   
   db().adjust_balance( o.issue_to_account, o.asset_to_issue );

   db().modify( *asset_dyn_data, [&]( asset_dynamic_data_object& data ){
        data.current_supply += o.asset_to_issue.amount;
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_reserve_evaluator::do_evaluate( const asset_reserve_operation& o )
{ try {
   const database& d = db();
   FC_ASSERT( d.head_block_time() >= HARDFORK_0_3_TIME, "Can only be asset_reserve after HARDFORK_0_3_TIME" );

   const asset_object& a = d.get_asset_by_aid( o.amount_to_reserve.asset_id );

   from_account = &d.get_account_by_uid( o.payer );
   FC_ASSERT( is_authorized_asset( d, *from_account, a ) );

   asset_dyn_data = &a.dynamic_asset_data_id(d);
   FC_ASSERT( (asset_dyn_data->current_supply - o.amount_to_reserve.amount) >= 0 );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_reserve_evaluator::do_apply( const asset_reserve_operation& o )
{ try {
   
   db().adjust_balance( o.payer, -o.amount_to_reserve );

   db().modify( *asset_dyn_data, [&]( asset_dynamic_data_object& data ){
        data.current_supply -= o.amount_to_reserve.amount;
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_update_evaluator::do_evaluate(const asset_update_operation& o)
{ try {

   database& d = db();
   FC_ASSERT( d.head_block_time() >= HARDFORK_0_3_TIME, "Can only be asset_update after HARDFORK_0_3_TIME" );

   const asset_object& a = d.get_asset_by_aid( o.asset_to_update );
   auto a_copy = a;
   a_copy.options = o.new_options;
   a_copy.validate();

   if( !a.can_change_max_supply() && !a_copy.can_change_max_supply() )
   {
      FC_ASSERT( a.options.max_supply == o.new_options.max_supply, "'change_max_supply' flag is disabled for this asset" );
   }

   if( o.new_precision.valid() )
   {
      FC_ASSERT( *o.new_precision != a.precision, "new precision should not be equal to current precision" );
   }

   if( a.dynamic_asset_data_id(d).current_supply != 0 )
   {
      FC_ASSERT( !o.new_precision.valid(), "Cannot change asset precision when current supply is not zero" );

      // new issuer_permissions must be subset of old issuer permissions
      FC_ASSERT(!(~o.new_options.issuer_permissions & a.options.issuer_permissions),
                "Cannot reinstate previously revoked issuer permissions on an asset.");
   }

   // changed flags must be subset of old issuer permissions
   FC_ASSERT(!((o.new_options.flags ^ a.options.flags) & a.options.issuer_permissions),
             "Flag change is forbidden by issuer permissions");

   asset_to_update = &a;
   FC_ASSERT( o.issuer == a.issuer, "", ("o.issuer", o.issuer)("a.issuer", a.issuer) );

   const auto& chain_parameters = d.get_global_properties().parameters;

   FC_ASSERT( o.new_options.whitelist_authorities.size() <= chain_parameters.maximum_asset_whitelist_authorities );
   for( auto id : o.new_options.whitelist_authorities )
      d.get_account_by_uid(id);
   FC_ASSERT( o.new_options.blacklist_authorities.size() <= chain_parameters.maximum_asset_whitelist_authorities );
   for( auto id : o.new_options.blacklist_authorities )
      d.get_account_by_uid(id);

   return void_result();
} FC_CAPTURE_AND_RETHROW((o)) }

void_result asset_update_evaluator::do_apply(const asset_update_operation& o)
{ try {
   database& d = db();

   d.modify(*asset_to_update, [&](asset_object& a) {
      if( o.new_precision.valid() )
         a.precision = *o.new_precision;
      a.options = o.new_options;
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result asset_claim_fees_evaluator::do_evaluate( const asset_claim_fees_operation& o )
{ try {
   database& d = db();
   FC_ASSERT( d.head_block_time() >= HARDFORK_0_3_TIME, "Can only be asset_claim after HARDFORK_0_3_TIME" );
   
   const asset_object& asset = d.get_asset_by_aid( o.amount_to_claim.asset_id );
   FC_ASSERT( asset.issuer == o.issuer, "Asset fees may only be claimed by the issuer" );
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }


void_result asset_claim_fees_evaluator::do_apply( const asset_claim_fees_operation& o )
{ try {
   database& d = db();

   const asset_object& a = d.get_asset_by_aid( o.amount_to_claim.asset_id );
   const asset_dynamic_data_object& addo = a.dynamic_asset_data_id(d);
   FC_ASSERT( o.amount_to_claim.amount <= addo.accumulated_fees, "Attempt to claim more fees than have accumulated", ("addo",addo) );

   d.modify( addo, [&]( asset_dynamic_data_object& _addo  ) {
     _addo.accumulated_fees -= o.amount_to_claim.amount;
   });

   d.adjust_balance( o.issuer, o.amount_to_claim );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }


} } // graphene::chain
