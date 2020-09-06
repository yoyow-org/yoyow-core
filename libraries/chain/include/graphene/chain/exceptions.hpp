/*
 * Copyright (c)2015 Cryptonomex, Inc., and contributors.
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
#pragma once

#include <fc/exception/exception.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/protocol/types.hpp>
//#include <graphene/chain/protocol/protocol.hpp>

#define GRAPHENE_ASSERT( expr, exc_type, FORMAT, ... )\
   FC_MULTILINE_MACRO_BEGIN                                           \
   if( !(expr))\
      FC_THROW_EXCEPTION( exc_type, FORMAT, __VA_ARGS__ );            \
   FC_MULTILINE_MACRO_END

#define GRAPHENE_THROW( exc_type, FORMAT, ... ) \
    throw exc_type( FC_LOG_MESSAGE( error, FORMAT, __VA_ARGS__ ) );

#define GRAPHENE_DECLARE_OP_BASE_EXCEPTIONS( op_name )\
   FC_DECLARE_DERIVED_EXCEPTION(                                      \
      op_name ## _validate_exception,                                 \
      graphene::chain::operation_validate_exception,                  \
      3040000 + 100 * operation::tag< op_name ## _operation >::value  \
      )\
   FC_DECLARE_DERIVED_EXCEPTION(                                      \
      op_name ## _evaluate_exception,                                 \
      graphene::chain::operation_evaluate_exception,                  \
      3050000 + 100 * operation::tag< op_name ## _operation >::value  \
      )

#define GRAPHENE_IMPLEMENT_OP_BASE_EXCEPTIONS( op_name )\
   FC_IMPLEMENT_DERIVED_EXCEPTION(                                    \
      op_name ## _validate_exception,                                 \
      graphene::chain::operation_validate_exception,                  \
      3040000 + 100 * operation::tag< op_name ## _operation >::value, \
      #op_name "_operation validation exception"                      \
      )\
   FC_IMPLEMENT_DERIVED_EXCEPTION(                                    \
      op_name ## _evaluate_exception,                                 \
      graphene::chain::operation_evaluate_exception,                  \
      3050000 + 100 * operation::tag< op_name ## _operation >::value, \
      #op_name "_operation evaluation exception"                      \
      )

#define GRAPHENE_DECLARE_OP_VALIDATE_EXCEPTION( exc_name, op_name, seqnum )\
   FC_DECLARE_DERIVED_EXCEPTION(                                      \
      op_name ## _ ## exc_name,                                       \
      graphene::chain::op_name ## _validate_exception,                \
      3040000 + 100 * operation::tag< op_name ## _operation >::value  \
         + seqnum                                                     \
      )

#define GRAPHENE_IMPLEMENT_OP_VALIDATE_EXCEPTION( exc_name, op_name, seqnum, msg )\
   FC_IMPLEMENT_DERIVED_EXCEPTION(                                    \
      op_name ## _ ## exc_name,                                       \
      graphene::chain::op_name ## _validate_exception,                \
      3040000 + 100 * operation::tag< op_name ## _operation >::value  \
         + seqnum,                                                    \
      msg                                                             \
      )

#define GRAPHENE_DECLARE_OP_EVALUATE_EXCEPTION( exc_name, op_name, seqnum )\
   FC_DECLARE_DERIVED_EXCEPTION(                                      \
      op_name ## _ ## exc_name,                                       \
      graphene::chain::op_name ## _evaluate_exception,                \
      3050000 + 100 * operation::tag< op_name ## _operation >::value  \
         + seqnum                                                     \
      )

#define GRAPHENE_IMPLEMENT_OP_EVALUATE_EXCEPTION( exc_name, op_name, seqnum, msg )\
   FC_IMPLEMENT_DERIVED_EXCEPTION(                                    \
      op_name ## _ ## exc_name,                                       \
      graphene::chain::op_name ## _evaluate_exception,                \
      3050000 + 100 * operation::tag< op_name ## _operation >::value  \
         + seqnum,                                                    \
      msg                                                             \
      )

#define GRAPHENE_TRY_NOTIFY( signal, ... )\
   try                                                                        \
   {                                                                          \
      signal( __VA_ARGS__ );                                                  \
   }                                                                          \
   catch( const graphene::chain::plugin_exception& e )\
   {                                                                          \
      elog( "Caught plugin exception: ${e}", ("e", e.to_detail_string()));  \
      throw;                                                                  \
   }                                                                          \
   catch( ... )\
   {                                                                          \
      wlog( "Caught unexpected exception in plugin" );                        \
   }




namespace graphene { namespace chain {

   FC_DECLARE_EXCEPTION( chain_exception, 3000000 );

   FC_DECLARE_DERIVED_EXCEPTION( chain_type_exception,              chain_exception, 3010000);

   FC_DECLARE_DERIVED_EXCEPTION( name_type_exception,               chain_type_exception, 3010001);
   FC_DECLARE_DERIVED_EXCEPTION( public_key_type_exception,         chain_type_exception, 3010002);
   FC_DECLARE_DERIVED_EXCEPTION( private_key_type_exception,        chain_type_exception, 3010003);
   FC_DECLARE_DERIVED_EXCEPTION( authority_type_exception,          chain_type_exception, 3010004);
   FC_DECLARE_DERIVED_EXCEPTION( action_type_exception,             chain_type_exception, 3010005);
   FC_DECLARE_DERIVED_EXCEPTION( transaction_type_exception,        chain_type_exception, 3010006);
   FC_DECLARE_DERIVED_EXCEPTION( abi_type_exception,                chain_type_exception, 3010007);
   FC_DECLARE_DERIVED_EXCEPTION( block_id_type_exception,           chain_type_exception, 3010008);
   FC_DECLARE_DERIVED_EXCEPTION( transaction_id_type_exception,     chain_type_exception, 3010009);
   FC_DECLARE_DERIVED_EXCEPTION( packed_transaction_type_exception, chain_type_exception, 3010010);
   FC_DECLARE_DERIVED_EXCEPTION( asset_type_exception,              chain_type_exception, 3010011);
   FC_DECLARE_DERIVED_EXCEPTION( database_query_exception,          chain_exception, 3010000);
   FC_DECLARE_DERIVED_EXCEPTION( block_validate_exception,          chain_exception, 3020000);
   FC_DECLARE_DERIVED_EXCEPTION( transaction_exception,             chain_exception, 3030000);
   FC_DECLARE_DERIVED_EXCEPTION( operation_validate_exception,      chain_exception, 3040000);
   FC_DECLARE_DERIVED_EXCEPTION( operation_evaluate_exception,      chain_exception, 3050000);
   FC_DECLARE_DERIVED_EXCEPTION( utility_exception,                 chain_exception, 3060000);
   FC_DECLARE_DERIVED_EXCEPTION( undo_database_exception,           chain_exception, 3070000);
   FC_DECLARE_DERIVED_EXCEPTION( unlinkable_block_exception,        chain_exception, 3080000);
   FC_DECLARE_DERIVED_EXCEPTION( black_swan_exception,              chain_exception, 3090000);
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_active_auth,            transaction_exception, 3030001);
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_owner_auth,             transaction_exception, 3030002);
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_other_auth,             transaction_exception, 3030003);
   FC_DECLARE_DERIVED_EXCEPTION( tx_irrelevant_sig,                 transaction_exception, 3030004);
   FC_DECLARE_DERIVED_EXCEPTION( tx_duplicate_sig,                  transaction_exception, 3030005);
   FC_DECLARE_DERIVED_EXCEPTION( invalid_committee_approval,        transaction_exception, 3030006);
   FC_DECLARE_DERIVED_EXCEPTION( insufficient_fee,                  transaction_exception, 3030007);
   FC_DECLARE_DERIVED_EXCEPTION( tx_missing_secondary_auth,         transaction_exception, 3030008);
   FC_DECLARE_DERIVED_EXCEPTION( invalid_pts_address,               utility_exception, 3060001);
   FC_DECLARE_DERIVED_EXCEPTION( insufficient_feeds,                chain_exception, 37006);
   FC_DECLARE_DERIVED_EXCEPTION( pop_empty_chain,                   undo_database_exception, 3070001);
   FC_DECLARE_DERIVED_EXCEPTION( wasm_exception,                    chain_exception, 3070000);
   FC_DECLARE_DERIVED_EXCEPTION( page_memory_error,                 wasm_exception, 3070001);
   FC_DECLARE_DERIVED_EXCEPTION( wasm_execution_error,              wasm_exception, 3070002);
   FC_DECLARE_DERIVED_EXCEPTION( wasm_serialization_error,          wasm_exception, 3070003);
   FC_DECLARE_DERIVED_EXCEPTION( overlapping_memory_error,          wasm_exception, 3070004);
   FC_DECLARE_DERIVED_EXCEPTION( action_validate_exception,         chain_exception, 3070005);
   FC_DECLARE_DERIVED_EXCEPTION( account_name_exists_exception,     action_validate_exception, 3070006);
   FC_DECLARE_DERIVED_EXCEPTION( invalid_action_args_exception,     action_validate_exception, 3070007);
   FC_DECLARE_DERIVED_EXCEPTION( graphene_assert_message_exception, action_validate_exception, 3070008);
   FC_DECLARE_DERIVED_EXCEPTION( graphene_assert_code_exception,    action_validate_exception, 3070009);
   FC_DECLARE_DERIVED_EXCEPTION( wabt_execution_error,              wasm_exception, 3070020);
   FC_DECLARE_DERIVED_EXCEPTION( resource_exhausted_exception, chain_exception, 3080000);
   FC_DECLARE_DERIVED_EXCEPTION( ram_usage_exceeded, resource_exhausted_exception, 3080001);
   FC_DECLARE_DERIVED_EXCEPTION( tx_net_usage_exceeded, resource_exhausted_exception, 3080002);
   FC_DECLARE_DERIVED_EXCEPTION( block_net_usage_exceeded, resource_exhausted_exception, 3080003);
   FC_DECLARE_DERIVED_EXCEPTION( tx_cpu_usage_exceeded, resource_exhausted_exception, 3080004);
   FC_DECLARE_DERIVED_EXCEPTION( block_cpu_usage_exceeded, resource_exhausted_exception, 3080005);
   FC_DECLARE_DERIVED_EXCEPTION( deadline_exception, resource_exhausted_exception, 3080006);
   FC_DECLARE_DERIVED_EXCEPTION( abi_not_found_exception, chain_type_exception, 3010008);
   FC_DECLARE_DERIVED_EXCEPTION( table_not_found_exception, chain_type_exception, 3010009);
   FC_DECLARE_DERIVED_EXCEPTION( contract_not_found_exception, chain_type_exception, 3010010);
   FC_DECLARE_DERIVED_EXCEPTION( leeway_deadline_exception, deadline_exception, 3081001);
   
   GRAPHENE_DECLARE_OP_BASE_EXCEPTIONS( transfer );
   GRAPHENE_DECLARE_OP_EVALUATE_EXCEPTION( from_account_not_whitelisted, transfer, 1);
   GRAPHENE_DECLARE_OP_EVALUATE_EXCEPTION( to_account_not_whitelisted, transfer, 2);
   GRAPHENE_DECLARE_OP_EVALUATE_EXCEPTION( restricted_transfer_asset, transfer, 3);
   GRAPHENE_DECLARE_OP_EVALUATE_EXCEPTION( restricted_transfer_to_contract, transfer,4);
   
   GRAPHENE_DECLARE_OP_BASE_EXCEPTIONS( account_create );
   GRAPHENE_DECLARE_OP_EVALUATE_EXCEPTION( max_auth_exceeded, account_create, 1);
   GRAPHENE_DECLARE_OP_EVALUATE_EXCEPTION( auth_account_not_found, account_create, 2   );
   GRAPHENE_DECLARE_OP_BASE_EXCEPTIONS( account_update_auth );

   GRAPHENE_DECLARE_OP_EVALUATE_EXCEPTION( max_auth_exceeded, account_update_auth, 1);
   GRAPHENE_DECLARE_OP_EVALUATE_EXCEPTION( account_not_found, account_update_auth, 2);
   
   GRAPHENE_DECLARE_OP_BASE_EXCEPTIONS( asset_reserve );
   GRAPHENE_DECLARE_OP_EVALUATE_EXCEPTION( invalid_on_mia, asset_reserve, 1);
   
   GRAPHENE_DECLARE_OP_BASE_EXCEPTIONS( proposal_create );
   GRAPHENE_DECLARE_OP_EVALUATE_EXCEPTION( review_period_required, proposal_create, 1);
   GRAPHENE_DECLARE_OP_EVALUATE_EXCEPTION( review_period_insufficient, proposal_create, 2);
   
   GRAPHENE_DECLARE_OP_BASE_EXCEPTIONS( override_transfer );
   GRAPHENE_DECLARE_OP_EVALUATE_EXCEPTION( not_permitted, override_transfer, 1);

   FC_DECLARE_EXCEPTION( abi_generation_exception, 999999 );

   #define GRAPHENE_RECODE_EXC( cause_type, effect_type )\
      catch( const cause_type& e )\
      { throw( effect_type( e.what(), e.get_log())); }

} } // graphene::chain
