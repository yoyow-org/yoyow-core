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

#include <graphene/db/flat_index.hpp>
#include <graphene/chain/database.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/block_summary_object.hpp>
#include <graphene/chain/chain_property_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/content_object.hpp>
#include <graphene/chain/csaf_object.hpp>
#include <graphene/chain/global_property_object.hpp>
#include <graphene/chain/operation_history_object.hpp>
#include <graphene/chain/proposal_object.hpp>
#include <graphene/chain/transaction_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/witness_schedule_object.hpp>

#include <graphene/chain/account_evaluator.hpp>
#include <graphene/chain/asset_evaluator.hpp>
#include <graphene/chain/committee_member_evaluator.hpp>
#include <graphene/chain/content_evaluator.hpp>
#include <graphene/chain/csaf_evaluator.hpp>
#include <graphene/chain/proposal_evaluator.hpp>
#include <graphene/chain/transfer_evaluator.hpp>
#include <graphene/chain/witness_evaluator.hpp>

#include <graphene/chain/protocol/fee_schedule.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/uint128.hpp>
#include <fc/crypto/digest.hpp>

#include <boost/algorithm/string.hpp>

namespace graphene { namespace chain {

// C++ requires that static class variables declared and initialized
// in headers must also have a definition in a single source file,
// else linker errors will occur [1].
//
// The purpose of this source file is to collect such definitions in
// a single place.
//
// [1] http://stackoverflow.com/questions/8016780/undefined-reference-to-static-constexpr-char

const uint8_t account_object::space_id;
const uint8_t account_object::type_id;

const uint8_t asset_object::space_id;
const uint8_t asset_object::type_id;

const uint8_t block_summary_object::space_id;
const uint8_t block_summary_object::type_id;

const uint8_t committee_member_object::space_id;
const uint8_t committee_member_object::type_id;

const uint8_t committee_proposal_object::space_id;
const uint8_t committee_proposal_object::type_id;

const uint8_t csaf_lease_object::space_id;
const uint8_t csaf_lease_object::type_id;

const uint8_t global_property_object::space_id;
const uint8_t global_property_object::type_id;

const uint8_t operation_history_object::space_id;
const uint8_t operation_history_object::type_id;

const uint8_t platform_object::space_id;
const uint8_t platform_object::type_id;

const uint8_t platform_vote_object::space_id;
const uint8_t platform_vote_object::type_id;

const uint8_t post_object::space_id;
const uint8_t post_object::type_id;

const uint8_t active_post_object::space_id;
const uint8_t active_post_object::type_id;

const uint8_t proposal_object::space_id;
const uint8_t proposal_object::type_id;

const uint8_t registrar_takeover_object::space_id;
const uint8_t registrar_takeover_object::type_id;

const uint8_t transaction_object::space_id;
const uint8_t transaction_object::type_id;

const uint8_t voter_object::space_id;
const uint8_t voter_object::type_id;

const uint8_t witness_object::space_id;
const uint8_t witness_object::type_id;

const uint8_t witness_vote_object::space_id;
const uint8_t witness_vote_object::type_id;

const uint8_t committee_member_vote_object::space_id;
const uint8_t committee_member_vote_object::type_id;

void database::initialize_evaluators()
{
   _operation_evaluators.resize(255);
   register_evaluator<account_create_evaluator>();
   register_evaluator<account_manage_evaluator>();
   register_evaluator<account_update_key_evaluator>();
   register_evaluator<account_update_auth_evaluator>();
   register_evaluator<account_update_proxy_evaluator>();
   register_evaluator<account_auth_platform_evaluator>();
   register_evaluator<account_cancel_auth_platform_evaluator>();
   register_evaluator<account_enable_allowed_assets_evaluator>();
   register_evaluator<account_update_allowed_assets_evaluator>();
   register_evaluator<account_whitelist_evaluator>();
   register_evaluator<committee_member_create_evaluator>();
   register_evaluator<committee_member_update_evaluator>();
   register_evaluator<committee_member_vote_update_evaluator>();
   register_evaluator<committee_proposal_create_evaluator>();
   register_evaluator<committee_proposal_update_evaluator>();
   register_evaluator<platform_create_evaluator>();
   register_evaluator<platform_update_evaluator>();
   register_evaluator<platform_vote_update_evaluator>();
   register_evaluator<post_evaluator>();
   register_evaluator<post_update_evaluator>();
   register_evaluator<csaf_collect_evaluator>();
   register_evaluator<csaf_lease_evaluator>();
   register_evaluator<asset_create_evaluator>();
   register_evaluator<asset_issue_evaluator>();
   register_evaluator<asset_reserve_evaluator>();
   register_evaluator<asset_update_evaluator>();
   register_evaluator<asset_claim_fees_evaluator>();
   register_evaluator<transfer_evaluator>();
   register_evaluator<override_transfer_evaluator>();
   register_evaluator<proposal_create_evaluator>();
   register_evaluator<proposal_update_evaluator>();
   register_evaluator<proposal_delete_evaluator>();
   register_evaluator<witness_create_evaluator>();
   register_evaluator<witness_update_evaluator>();
   register_evaluator<witness_vote_update_evaluator>();
   register_evaluator<witness_collect_pay_evaluator>();
   register_evaluator<witness_report_evaluator>();
}

void database::initialize_indexes()
{
   reset_indexes();
   _undo_db.set_max_size( GRAPHENE_MIN_UNDO_HISTORY );

   //Protocol object indexes
   add_index< primary_index<asset_index> >();

   auto acnt_index = add_index< primary_index<account_index> >();
   acnt_index->add_secondary_index<account_member_index>();
   acnt_index->add_secondary_index<account_referrer_index>();

   add_index< primary_index<platform_index> >();
   add_index< primary_index<post_index> >();
	 add_index< primary_index<active_post_index> >();

   add_index< primary_index<committee_member_index> >();
   add_index< primary_index<committee_proposal_index> >();
   add_index< primary_index<witness_index> >();

   auto prop_index = add_index< primary_index<proposal_index > >();
   prop_index->add_secondary_index<required_approval_index>();

   //Implementation object indexes
   add_index< primary_index<transaction_index                             > >();
   add_index< primary_index<account_balance_index                         > >();
   add_index< primary_index<simple_index<global_property_object          >> >();
   add_index< primary_index<simple_index<dynamic_global_property_object  >> >();
   add_index< primary_index<account_statistics_index                      > >();
   add_index< primary_index<voter_index                                   > >();
   add_index< primary_index<registrar_takeover_index                      > >();
   add_index< primary_index<witness_vote_index                            > >();
   add_index< primary_index<platform_vote_index                           > >();
   add_index< primary_index<committee_member_vote_index                   > >();
   add_index< primary_index<csaf_lease_index                              > >();
   add_index< primary_index<simple_index<asset_dynamic_data_object       >> >();
   add_index< primary_index<flat_index<  block_summary_object            >> >();
   add_index< primary_index<simple_index<chain_property_object          > > >();
   add_index< primary_index<simple_index<witness_schedule_object        > > >();

}

void database::init_genesis(const genesis_state_type& genesis_state)
{ try {
   FC_ASSERT( genesis_state.initial_timestamp != time_point_sec(), "Must initialize genesis timestamp." );
   FC_ASSERT( genesis_state.initial_timestamp.sec_since_epoch() % GRAPHENE_DEFAULT_BLOCK_INTERVAL == 0,
              "Genesis timestamp must be divisible by GRAPHENE_DEFAULT_BLOCK_INTERVAL." );
   FC_ASSERT(genesis_state.initial_witness_candidates.size() > 0,
             "Cannot start a chain with zero witnesses.");
   FC_ASSERT(genesis_state.initial_active_witnesses <= genesis_state.initial_witness_candidates.size(),
             "initial_active_witnesses is larger than the number of candidate witnesses.");

   _undo_db.disable();
   struct auth_inhibitor {
      auth_inhibitor(database& db) : db(db), old_flags(db.node_properties().skip_flags)
      { db.node_properties().skip_flags |= skip_authority_check; }
      ~auth_inhibitor()
      { db.node_properties().skip_flags = old_flags; }
   private:
      database& db;
      uint32_t old_flags;
   } inhibitor(*this);

   transaction_evaluation_state genesis_eval_state(this);

   flat_index<block_summary_object>& bsi = get_mutable_index_type< flat_index<block_summary_object> >();
   bsi.resize(0xffff+1);

   // Create blockchain accounts
   fc::ecc::private_key null_private_key = fc::ecc::private_key::regenerate(fc::sha256::hash(string("null_key")));
   create<account_balance_object>([](account_balance_object& b) {
      b.owner = GRAPHENE_COMMITTEE_ACCOUNT_UID;
      b.asset_type = GRAPHENE_CORE_ASSET_AID;
      b.balance = GRAPHENE_MAX_SHARE_SUPPLY;
   });
   FC_ASSERT(create<account_object>([this](account_object& a) {
       a.uid = GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID;
       a.name = "proxy-to-self";
       a.statistics = create<account_statistics_object>([&](account_statistics_object& s){s.owner = a.uid;}).id;
       a.owner.weight_threshold = 1;
       a.active.weight_threshold = 1;
       a.secondary.weight_threshold = 1;
       a.registrar = a.lifetime_referrer = a.referrer = GRAPHENE_NULL_ACCOUNT_UID;
       a.membership_expiration_date = time_point_sec::maximum();
       a.network_fee_percentage = 0;
       a.lifetime_referrer_fee_percentage = GRAPHENE_100_PERCENT;
   }).get_id() == GRAPHENE_PROXY_TO_SELF_ACCOUNT);
   const account_object& committee_account =
      create<account_object>( [&](account_object& n) {
         n.uid = GRAPHENE_COMMITTEE_ACCOUNT_UID;
         n.membership_expiration_date = time_point_sec::maximum();
         n.network_fee_percentage = GRAPHENE_DEFAULT_NETWORK_PERCENT_OF_FEE;
         n.lifetime_referrer_fee_percentage = GRAPHENE_100_PERCENT - GRAPHENE_DEFAULT_NETWORK_PERCENT_OF_FEE;
         n.owner.weight_threshold = 1;
         n.active.weight_threshold = 1;
         n.secondary.weight_threshold = 1;
         n.name = "committee-account";
         n.statistics = create<account_statistics_object>( [&](account_statistics_object& s){
            s.owner = n.uid;
            s.core_balance = GRAPHENE_MAX_SHARE_SUPPLY;
         }).id;
      });
   FC_ASSERT(committee_account.get_id() == GRAPHENE_COMMITTEE_ACCOUNT);
   FC_ASSERT(create<account_object>([this](account_object& a) {
       a.uid = GRAPHENE_WITNESS_ACCOUNT_UID;
       a.name = "witness-account";
       a.statistics = create<account_statistics_object>([&](account_statistics_object& s){s.owner = a.uid;}).id;
       a.owner.weight_threshold = 1;
       a.active.weight_threshold = 1;
       a.secondary.weight_threshold = 1;
       a.registrar = a.lifetime_referrer = a.referrer = GRAPHENE_WITNESS_ACCOUNT_UID;
       a.membership_expiration_date = time_point_sec::maximum();
       a.network_fee_percentage = GRAPHENE_DEFAULT_NETWORK_PERCENT_OF_FEE;
       a.lifetime_referrer_fee_percentage = GRAPHENE_100_PERCENT - GRAPHENE_DEFAULT_NETWORK_PERCENT_OF_FEE;
   }).get_id() == GRAPHENE_WITNESS_ACCOUNT);
   FC_ASSERT(create<account_object>([this](account_object& a) {
       a.uid = GRAPHENE_RELAXED_COMMITTEE_ACCOUNT_UID;
       a.name = "relaxed-committee-account";
       a.statistics = create<account_statistics_object>([&](account_statistics_object& s){s.owner = a.uid;}).id;
       a.owner.weight_threshold = 1;
       a.active.weight_threshold = 1;
       a.secondary.weight_threshold = 1;
       a.registrar = a.lifetime_referrer = a.referrer = GRAPHENE_RELAXED_COMMITTEE_ACCOUNT_UID;
       a.membership_expiration_date = time_point_sec::maximum();
       a.network_fee_percentage = GRAPHENE_DEFAULT_NETWORK_PERCENT_OF_FEE;
       a.lifetime_referrer_fee_percentage = GRAPHENE_100_PERCENT - GRAPHENE_DEFAULT_NETWORK_PERCENT_OF_FEE;
   }).get_id() == GRAPHENE_RELAXED_COMMITTEE_ACCOUNT);
   FC_ASSERT(create<account_object>([this](account_object& a) {
       a.uid = GRAPHENE_NULL_ACCOUNT_UID;
       a.name = "null-account";
       a.statistics = create<account_statistics_object>([&](account_statistics_object& s){s.owner = a.uid;}).id;
       a.owner.weight_threshold = 1;
       a.active.weight_threshold = 1;
       a.secondary.weight_threshold = 1;
       a.is_registrar = true;
       a.is_full_member = true;
       a.registrar = a.lifetime_referrer = a.referrer = GRAPHENE_NULL_ACCOUNT_UID;
       a.membership_expiration_date = time_point_sec::maximum();
       a.network_fee_percentage = 0;
       a.lifetime_referrer_fee_percentage = GRAPHENE_100_PERCENT;
   }).get_id() == GRAPHENE_NULL_ACCOUNT);
   FC_ASSERT(create<account_object>([this](account_object& a) {
       a.uid = GRAPHENE_TEMP_ACCOUNT_UID;
       a.name = "temp-account";
       a.statistics = create<account_statistics_object>([&](account_statistics_object& s){s.owner = a.uid;}).id;
       a.owner.weight_threshold = 0;
       a.active.weight_threshold = 0;
       a.secondary.weight_threshold = 0;
       a.registrar = a.lifetime_referrer = a.referrer = GRAPHENE_NULL_ACCOUNT_UID;
       a.membership_expiration_date = time_point_sec::maximum();
       a.network_fee_percentage = GRAPHENE_DEFAULT_NETWORK_PERCENT_OF_FEE;
       a.lifetime_referrer_fee_percentage = GRAPHENE_100_PERCENT - GRAPHENE_DEFAULT_NETWORK_PERCENT_OF_FEE;
   }).get_id() == GRAPHENE_TEMP_ACCOUNT);

   // Create core asset
   const asset_dynamic_data_object& dyn_asset =
      create<asset_dynamic_data_object>([&](asset_dynamic_data_object& a) {
         a.asset_id = GRAPHENE_CORE_ASSET_AID;
         a.current_supply = GRAPHENE_MAX_SHARE_SUPPLY;
      });
   const asset_object& core_asset =
     create<asset_object>( [&]( asset_object& a ) {
         a.asset_id = GRAPHENE_CORE_ASSET_AID;
         a.symbol = GRAPHENE_SYMBOL;
         a.options.max_supply = genesis_state.max_core_supply;
         a.precision = GRAPHENE_BLOCKCHAIN_PRECISION_DIGITS;
         a.options.flags = 0;
         a.options.issuer_permissions = 0; // owned by null-account, doesn't matter what permission it has
         a.issuer = GRAPHENE_NULL_ACCOUNT_UID;
         a.dynamic_asset_data_id = dyn_asset.id;
      });
   FC_ASSERT( object_id_type(core_asset.id).instance() == core_asset.asset_id );
   FC_ASSERT( core_asset.asset_id == asset().asset_id );
   FC_ASSERT( get_balance(GRAPHENE_COMMITTEE_ACCOUNT_UID, GRAPHENE_CORE_ASSET_AID) == asset(dyn_asset.current_supply) );

   chain_id_type chain_id = genesis_state.compute_chain_id();

   // Create global properties
   create<global_property_object>([&](global_property_object& p) {
       p.parameters = genesis_state.initial_parameters;
       // Set fees to zero initially, so that genesis initialization needs not pay them
       // We'll fix it at the end of the function
       p.parameters.current_fees->zero_all_fees();

   });
   create<dynamic_global_property_object>([&](dynamic_global_property_object& p) {
      p.time = genesis_state.initial_timestamp;
      p.dynamic_flags = 0;
      p.witness_budget = 0;
      p.recent_slots_filled = fc::uint128::max_value();
   });

   FC_ASSERT( (genesis_state.immutable_parameters.min_witness_count & 1) == 1, "min_witness_count must be odd" );
   FC_ASSERT( (genesis_state.immutable_parameters.min_committee_member_count & 1) == 1, "min_committee_member_count must be odd" );

   create<chain_property_object>([&](chain_property_object& p)
   {
      p.chain_id = chain_id;
      p.immutable_parameters = genesis_state.immutable_parameters;
   } );
   create<block_summary_object>([&](block_summary_object&) {});

   // Create initial accounts
   for( const auto& account : genesis_state.initial_accounts )
   {
      account_create_operation cop;
      cop.uid = account.uid;
      cop.name = account.name;
      //cop.registrar = GRAPHENE_TEMP_ACCOUNT;
      cop.owner = authority(1, account.owner_key, 1);
      auto tmp_active_key = account.owner_key;
      if( account.active_key == public_key_type() )
         cop.active = cop.owner;
      else
      {
         cop.active = authority(1, account.active_key, 1);
         tmp_active_key = account.active_key;
      }
      if( account.secondary_key == public_key_type() )
         cop.secondary = cop.active;
      else
         cop.secondary = authority(1, account.secondary_key, 1);
      if( account.memo_key == public_key_type() )
         cop.memo_key = tmp_active_key;
      else
         cop.memo_key = account.memo_key;
      account_id_type account_id(apply_operation(genesis_eval_state, cop).get<object_id_type>());

      modify( get( account_id ), [&account](account_object& a) {
         a.reg_info.registrar = account.registrar;
         a.is_registrar = account.is_registrar;
         a.is_full_member = account.is_full_member;
      });
   }

   // Helper function to get account ID by name
   const auto& accounts_by_name = get_index_type<account_index>().indices().get<by_name>();

   auto get_account_uid = [&accounts_by_name](const string& name) {
      auto itr = accounts_by_name.find(name);
      FC_ASSERT(itr != accounts_by_name.end(),
                "Unable to find account '${acct}'. Did you forget to add a record for it to initial_accounts?",
                ("acct", name));
      return itr->get_uid();
   };

   // Helper function to get asset ID by symbol
   const auto& assets_by_symbol = get_index_type<asset_index>().indices().get<by_symbol>();

   const auto get_asset_aid = [&assets_by_symbol](const string& symbol) {
      auto itr = assets_by_symbol.find(symbol);
      FC_ASSERT(itr != assets_by_symbol.end(),
                "Unable to find asset '${sym}'. Did you forget to add a record for it to initial_assets?",
                ("sym", symbol));
      return itr->asset_id;
   };

   map<asset_aid_type, share_type> total_supplies;

   // Create initial account balances
   for( const auto& handout : genesis_state.initial_account_balances )
   {
      const auto asset_id = get_asset_aid(handout.asset_symbol);
      adjust_balance( handout.uid, asset(handout.amount, asset_id) );
      total_supplies[ asset_id ] += handout.amount;
   }

   if( total_supplies[ GRAPHENE_CORE_ASSET_AID ] > 0 )
   {
       adjust_balance(GRAPHENE_COMMITTEE_ACCOUNT_UID, -get_balance(GRAPHENE_COMMITTEE_ACCOUNT_UID,GRAPHENE_CORE_ASSET_AID));
   }
   else
   {
       total_supplies[ GRAPHENE_CORE_ASSET_AID ] = GRAPHENE_MAX_SHARE_SUPPLY;
   }

   // Save tallied supplies
   for( const auto& item : total_supplies )
   {
       const auto asset_id = item.first;
       const auto total_supply = item.second;

       modify( get( get_asset_by_aid( asset_id ).dynamic_asset_data_id ), [ total_supply ]( asset_dynamic_data_object& addo ) {
           addo.current_supply = total_supply;
       } );
   }

   // Create special witness account
   const witness_object& wit = create<witness_object>([&](witness_object& w) {});
   FC_ASSERT( wit.id == GRAPHENE_NULL_WITNESS );
   remove(wit);

   // Create initial witnesses
   std::for_each(genesis_state.initial_witness_candidates.begin(), genesis_state.initial_witness_candidates.end(),
                 [&](const genesis_state_type::initial_witness_type& witness) {
      witness_create_operation op;
      op.account = get_account_uid(witness.owner_name);
      op.block_signing_key = witness.block_signing_key;
      apply_operation(genesis_eval_state, op);
   });

   // Create initial committee members
   std::for_each(genesis_state.initial_committee_candidates.begin(), genesis_state.initial_committee_candidates.end(),
                 [&](const genesis_state_type::initial_committee_member_type& member) {
      committee_member_create_operation op;
      op.account = get_account_uid(member.owner_name);
      apply_operation(genesis_eval_state, op);
   });

   // Create initial platforms
   std::for_each(genesis_state.initial_platforms.begin(), genesis_state.initial_platforms.end(),
                 [&](const genesis_state_type::initial_platform_type& platform) {
      /* TODO review
      const auto owner = get_account_by_uid(platform.owner);
      platform_create_operation op;
      op.account = platform.owner;
      op.name = platform.name;
      op.url = platform.url;
      apply_operation(genesis_eval_state, op);
      */
   });

   // Set active witnesses
   modify(get_global_properties(), [&](global_property_object& p) {
      for( uint32_t i = 1; i <= genesis_state.initial_active_witnesses; ++i )
      {
         p.active_witnesses.insert( std::make_pair( get( witness_id_type(i) ).account, scheduled_by_vote_top ) );
      }
   });

   // Enable fees
   modify(get_global_properties(), [&genesis_state](global_property_object& p) {
      p.parameters.current_fees = genesis_state.initial_parameters.current_fees;

      /* auto fees = fee_schedule::get_default();
      auto& cp = fees.parameters;
      for( const auto& f : genesis_state.initial_parameters.current_fees->parameters )
      {
         fee_parameters params; params.set_which(f.which());
         auto itr = cp.find(params);
         if( itr != cp.end() )
            *itr = f;
      }
      p.parameters.current_fees = fees; */
      //idump((p.parameters.current_fees));
   });

   // Update budgets
   adjust_budgets();

   // Update committee
   update_committee();

   // Create witness scheduler
   create<witness_schedule_object>([&]( witness_schedule_object& wso )
   {
      for( const auto& wid : get_global_properties().active_witnesses )
         wso.current_shuffled_witnesses.push_back( wid.first );
      wso.next_schedule_block_num = wso.current_shuffled_witnesses.size();
   });

   //debug_dump();

   _undo_db.enable();
} FC_CAPTURE_AND_RETHROW() }

} }
