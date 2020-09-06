/*
 * Copyright (c) 2019 BitShares Blockchain Foundation, and contributors.
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

#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/internal_exceptions.hpp>

namespace graphene { namespace chain {

   // Internal exceptions

   FC_IMPLEMENT_DERIVED_EXCEPTION( internal_exception, chain_exception, 3990000, "internal exception" )

   GRAPHENE_IMPLEMENT_INTERNAL_EXCEPTION( verify_auth_max_auth_exceeded, 1, "Exceeds max authority fan-out" )
   GRAPHENE_IMPLEMENT_INTERNAL_EXCEPTION( verify_auth_account_not_found, 2, "Auth account not found" )


   // Public exceptions
   FC_IMPLEMENT_EXCEPTION( chain_exception, 3000000, "blockchain exception" )

   FC_IMPLEMENT_DERIVED_EXCEPTION( chain_type_exception,              chain_exception, 3010000, "chain type exception" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( name_type_exception,               chain_type_exception, 3010001, "Invalid name" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( public_key_type_exception,         chain_type_exception, 3010002, "Invalid public key" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( private_key_type_exception,        chain_type_exception, 3010003, "Invalid private key" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( authority_type_exception,          chain_type_exception, 3010004, "Invalid authority" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( action_type_exception,             chain_type_exception, 3010005, "Invalid action" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( transaction_type_exception,        chain_type_exception, 3010006, "Invalid transaction" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( abi_type_exception,                chain_type_exception, 3010007, "Invalid ABI" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( block_id_type_exception,           chain_type_exception, 3010008, "Invalid block ID" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( transaction_id_type_exception,     chain_type_exception, 3010009, "Invalid transaction ID" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( packed_transaction_type_exception, chain_type_exception, 3010010, "Invalid packed transaction" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( asset_type_exception,              chain_type_exception, 3010011, "Invalid asset" )


   FC_IMPLEMENT_DERIVED_EXCEPTION( database_query_exception,          chain_exception, 3010000, "database query exception" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( block_validate_exception,          chain_exception, 3020000, "block validation exception" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( transaction_exception,             chain_exception, 3030000, "transaction validation exception" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( operation_validate_exception,      chain_exception, 3040000, "operation validation exception" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( operation_evaluate_exception,      chain_exception, 3050000, "operation evaluation exception" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( utility_exception,                 chain_exception, 3060000, "utility method exception" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( undo_database_exception,           chain_exception, 3070000, "undo database exception" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( unlinkable_block_exception,        chain_exception, 3080000, "unlinkable block" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( black_swan_exception,              chain_exception, 3090000, "black swan" )

   FC_IMPLEMENT_DERIVED_EXCEPTION( tx_missing_active_auth,            transaction_exception, 3030001, "missing required active authority" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( tx_missing_owner_auth,             transaction_exception, 3030002, "missing required owner authority" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( tx_missing_other_auth,             transaction_exception, 3030003, "missing required other authority" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( tx_irrelevant_sig,                 transaction_exception, 3030004, "irrelevant signature included" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( tx_duplicate_sig,                  transaction_exception, 3030005, "duplicate signature included" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( invalid_committee_approval,        transaction_exception, 3030006, "committee account cannot directly approve transaction" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( insufficient_fee,                  transaction_exception, 3030007, "insufficient fee" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( tx_missing_secondary_auth,         transaction_exception, 3030008, "missing required secondary authority" )

   FC_IMPLEMENT_DERIVED_EXCEPTION( invalid_pts_address,               utility_exception, 3060001, "invalid pts address" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( insufficient_feeds,                chain_exception, 37006, "insufficient feeds" )

   FC_IMPLEMENT_DERIVED_EXCEPTION( pop_empty_chain,                   undo_database_exception, 3070001, "there are no blocks to pop" )


   FC_IMPLEMENT_DERIVED_EXCEPTION( wasm_exception,                    chain_exception, 3070000, "WASM Exception" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( page_memory_error,                 wasm_exception, 3070001, "error in WASM page memory" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( wasm_execution_error,              wasm_exception, 3070002, "Runtime Error Processing WASM" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( wasm_serialization_error,          wasm_exception, 3070003, "Serialization Error Processing WASM" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( overlapping_memory_error,          wasm_exception, 3070004, "memcpy with overlapping memory" )

   FC_IMPLEMENT_DERIVED_EXCEPTION( action_validate_exception,         chain_exception, 3070005, "action exception" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( account_name_exists_exception,     action_validate_exception, 3070006, "account name already exists" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( invalid_action_args_exception,     action_validate_exception, 3070007, "Invalid Action Arguments" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( graphene_assert_message_exception, action_validate_exception, 3070008, "graphene_assert_message assertion failure" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( graphene_assert_code_exception,    action_validate_exception, 3070009, "graphene_assert_code assertion failure" )

   FC_IMPLEMENT_DERIVED_EXCEPTION( wabt_execution_error,              wasm_exception, 3070020, "Runtime Error Processing WABT" )

   FC_IMPLEMENT_DERIVED_EXCEPTION( resource_exhausted_exception, chain_exception, 3080000, "resource exhausted exception" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( ram_usage_exceeded, resource_exhausted_exception, 3080001, "account using more than allotted RAM usage" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( tx_net_usage_exceeded, resource_exhausted_exception, 3080002, "transaction exceeded the current network usage limit imposed on the transaction" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( block_net_usage_exceeded, resource_exhausted_exception, 3080003, "transaction network usage is too much for the remaining allowable usage of the current block" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( tx_cpu_usage_exceeded, resource_exhausted_exception, 3080004, "transaction exceeded the current CPU usage limit imposed on the transaction" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( block_cpu_usage_exceeded, resource_exhausted_exception, 3080005, "transaction CPU usage is too much for the remaining allowable usage of the current block" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( deadline_exception, resource_exhausted_exception, 3080006, "transaction took too long" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( abi_not_found_exception, chain_type_exception, 3010008, "No ABI found" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( table_not_found_exception, chain_type_exception, 3010009, "No table found" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( contract_not_found_exception, chain_type_exception, 3010010, "No contract found" )
   FC_IMPLEMENT_DERIVED_EXCEPTION( leeway_deadline_exception, deadline_exception, 3081001, "transaction reached the deadline set due to leeway on account CPU limits" )

   GRAPHENE_IMPLEMENT_OP_BASE_EXCEPTIONS( transfer );
   GRAPHENE_IMPLEMENT_OP_EVALUATE_EXCEPTION( from_account_not_whitelisted, transfer, 1, "owner mismatch" )
   GRAPHENE_IMPLEMENT_OP_EVALUATE_EXCEPTION( to_account_not_whitelisted, transfer, 2, "owner mismatch" )
   GRAPHENE_IMPLEMENT_OP_EVALUATE_EXCEPTION( restricted_transfer_asset, transfer, 3, "restricted transfer asset" )
   GRAPHENE_IMPLEMENT_OP_EVALUATE_EXCEPTION( restricted_transfer_to_contract, transfer, 4, "restricted transfer asset to contract account" )

   
   GRAPHENE_IMPLEMENT_OP_BASE_EXCEPTIONS( account_create );
   GRAPHENE_IMPLEMENT_OP_EVALUATE_EXCEPTION( max_auth_exceeded, account_create, 1, "Exceeds max authority fan-out" )
   GRAPHENE_IMPLEMENT_OP_EVALUATE_EXCEPTION( auth_account_not_found, account_create, 2, "Auth account not found" )
   
   GRAPHENE_IMPLEMENT_OP_BASE_EXCEPTIONS( account_update_auth );
   GRAPHENE_IMPLEMENT_OP_EVALUATE_EXCEPTION( max_auth_exceeded, account_update_auth, 1, "Exceeds max authority fan-out" )
   GRAPHENE_IMPLEMENT_OP_EVALUATE_EXCEPTION( account_not_found, account_update_auth, 2, "Auth account not found" )

   
   GRAPHENE_IMPLEMENT_OP_BASE_EXCEPTIONS( asset_reserve );
   GRAPHENE_IMPLEMENT_OP_EVALUATE_EXCEPTION( invalid_on_mia, asset_reserve, 1, "invalid on mia" )

  
   GRAPHENE_IMPLEMENT_OP_BASE_EXCEPTIONS( proposal_create );
   GRAPHENE_IMPLEMENT_OP_EVALUATE_EXCEPTION( review_period_required, proposal_create, 1, "review_period required" )
   GRAPHENE_IMPLEMENT_OP_EVALUATE_EXCEPTION( review_period_insufficient, proposal_create, 2, "review_period insufficient" )

  
   GRAPHENE_IMPLEMENT_OP_BASE_EXCEPTIONS( override_transfer );
   GRAPHENE_IMPLEMENT_OP_EVALUATE_EXCEPTION( not_permitted, override_transfer, 1, "not permitted" )

   FC_IMPLEMENT_EXCEPTION( abi_generation_exception, 999999, "Unable to generate abi" );


   #define GRAPHENE_RECODE_EXC( cause_type, effect_type ) \
      catch( const cause_type& e ) \
      { throw( effect_type( e.what(), e.get_log() ) ); }

} } // graphene::chain
