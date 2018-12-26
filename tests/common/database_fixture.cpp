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
#include <boost/test/unit_test.hpp>
#include <boost/program_options.hpp>

#include <graphene/account_history/account_history_plugin.hpp>
//#include <graphene/market_history/market_history_plugin.hpp>

#include <graphene/db/simple_index.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
//#include <graphene/chain/market_object.hpp>
//#include <graphene/chain/vesting_balance_object.hpp>
#include <graphene/chain/witness_object.hpp>

#include <graphene/utilities/tempdir.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/smart_ref_impl.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>

#include "database_fixture.hpp"

using namespace graphene::chain::test;

uint32_t GRAPHENE_TESTING_GENESIS_TIMESTAMP = 1530522000;// 1520611200;

namespace graphene { namespace chain {

using std::cout;
using std::cerr;

database_fixture::database_fixture()
   : app(), db( *app.chain_database() )
{
   try {
   int argc = boost::unit_test::framework::master_test_suite().argc;
   char** argv = boost::unit_test::framework::master_test_suite().argv;
   for( int i=1; i<argc; i++ )
   {
      const std::string arg = argv[i];
      if( arg == "--record-assert-trip" )
         fc::enable_record_assert_trip = true;
      if( arg == "--show-test-names" )
         std::cout << "running test " << boost::unit_test::framework::current_test_case().p_name << std::endl;
   }
   auto ahplugin = app.register_plugin<graphene::account_history::account_history_plugin>();
//   auto mhplugin = app.register_plugin<graphene::market_history::market_history_plugin>();
   init_account_pub_key = init_account_priv_key.get_public_key();

   boost::program_options::variables_map options;

   genesis_state.initial_timestamp = time_point_sec( GRAPHENE_TESTING_GENESIS_TIMESTAMP );

   genesis_state.initial_active_witnesses = 10;
   const int reserved_accounts = 10;
   for( int i = 0; i < genesis_state.initial_active_witnesses; ++i )
   {
      auto name = "init"+fc::to_string(i);
      genesis_state.initial_accounts.emplace_back(graphene::chain::calc_account_uid(i+reserved_accounts),
                                                  name,
                                                  0,
                                                  init_account_priv_key.get_public_key(),
                                                  init_account_priv_key.get_public_key(),
                                                  init_account_priv_key.get_public_key(),
                                                  init_account_priv_key.get_public_key(),
                                                  true);
      genesis_state.initial_committee_candidates.push_back({name});
      genesis_state.initial_witness_candidates.push_back({name, init_account_priv_key.get_public_key()});
   }
   genesis_state.initial_parameters.current_fees->zero_all_fees();
   open_database();

   // app.initialize();
   ahplugin->plugin_set_app(&app);
   ahplugin->plugin_initialize(options);
   //mhplugin->plugin_set_app(&app);
   //mhplugin->plugin_initialize(options);

   ahplugin->plugin_startup();
   //mhplugin->plugin_startup();

   generate_block();

   set_expiration( db, trx );
   } catch ( const fc::exception& e )
   {
      edump( (e.to_detail_string()) );
      throw;
   }

   return;
}

database_fixture::~database_fixture()
{ try {
   // If we're unwinding due to an exception, don't do any more checks.
   // This way, boost test's last checkpoint tells us approximately where the error was.
   if( !std::uncaught_exception() )
   {
      verify_asset_supplies(db);
      verify_account_history_plugin_index();
      BOOST_CHECK( db.get_node_properties().skip_flags == database::skip_nothing );
   }
   return;
} FC_CAPTURE_AND_RETHROW() }

fc::ecc::private_key database_fixture::generate_private_key(string seed)
{
   static const fc::ecc::private_key committee = fc::ecc::private_key::regenerate(fc::sha256::hash(string("null_key")));
   if( seed == "null_key" )
      return committee;
   return fc::ecc::private_key::regenerate(fc::sha256::hash(seed));
}

string database_fixture::generate_anon_acct_name()
{
   // names of the form "anon-acct-x123" ; the "x" is necessary
   //    to workaround issue #46
   return "anon-acct-x" + std::to_string( anon_acct_count++ );
}

void database_fixture::verify_asset_supplies( const database& db )
{
   //wlog("*** Begin asset supply verification ***");
   const asset_dynamic_data_object& core_asset_data = db.get_core_asset().dynamic_asset_data_id(db);

   const simple_index<account_statistics_object>& statistics_index = db.get_index_type<simple_index<account_statistics_object>>();
   const auto& balance_index = db.get_index_type<account_balance_index>().indices();
   map<asset_aid_type,share_type> total_balances;
   share_type core_in_orders;
   share_type reported_core_in_orders;

   for( const account_balance_object& b : balance_index )
      total_balances[b.asset_type] += b.balance;
   /*for (const account_statistics_object& a : statistics_index)
   {
   reported_core_in_orders += a.total_core_in_orders;
   total_balances[GRAPHENE_CORE_ASSET_AID] += a.pending_fees + a.pending_vested_fees;
   }
   for( const limit_order_object& o : db.get_index_type<limit_order_index>().indices() )
   {
   asset for_sale = o.amount_for_sale();
   if( for_sale.asset_id == GRAPHENE_CORE_ASSET_AID ) core_in_orders += for_sale.amount;
   total_balances[for_sale.asset_id] += for_sale.amount;
   total_balances[GRAPHENE_CORE_ASSET_AID] += o.deferred_fee;
   }*/
   for( const asset_object& asset_obj : db.get_index_type<asset_index>().indices() )
   {
      const auto& dasset_obj = asset_obj.dynamic_asset_data_id(db);
      total_balances[asset_obj.asset_id] += dasset_obj.accumulated_fees;
      //total_balances[asset_obj.asset_id] += dasset_obj.confidential_supply.value;
   }
   //for( const vesting_balance_object& vbo : db.get_index_type< vesting_balance_index >().indices() )
   //   total_balances[ vbo.balance.asset_id ] += vbo.balance.amount;

   total_balances[GRAPHENE_CORE_ASSET_AID] += db.get_dynamic_global_properties().witness_budget;

   for( const asset_object& asset_obj : db.get_index_type<asset_index>().indices() )
   {
      BOOST_CHECK_EQUAL(total_balances[asset_obj.asset_id].value, asset_obj.dynamic_asset_data_id(db).current_supply.value);
   }

   BOOST_CHECK_EQUAL( core_in_orders.value , reported_core_in_orders.value );
//   wlog("***  End  asset supply verification ***");
}

void database_fixture::verify_account_history_plugin_index( )const
{
   return;
   if( skip_key_index_test )
      return;

   const std::shared_ptr<graphene::account_history::account_history_plugin> pin =
      app.get_plugin<graphene::account_history::account_history_plugin>("account_history");
   if( pin->tracked_accounts().size() == 0 )
   {
      /*
      vector< pair< account_id_type, address > > tuples_from_db;
      const auto& primary_account_idx = db.get_index_type<account_index>().indices().get<by_id>();
      flat_set< public_key_type > acct_addresses;
      acct_addresses.reserve( 2 * GRAPHENE_DEFAULT_MAX_AUTHORITY_MEMBERSHIP + 2 );

      for( const account_object& acct : primary_account_idx )
      {
         account_id_type account_id = acct.id;
         acct_addresses.clear();
         for( const pair< account_id_type, weight_type >& auth : acct.owner.account_auths )
         {
            if( auth.first.type() == key_object_type )
               acct_addresses.insert(  auth.first );
         }
         for( const pair< object_id_type, weight_type >& auth : acct.active.auths )
         {
            if( auth.first.type() == key_object_type )
               acct_addresses.insert( auth.first );
         }
         acct_addresses.insert( acct.options.get_memo_key()(db).key_address() );
         for( const address& addr : acct_addresses )
            tuples_from_db.emplace_back( account_id, addr );
      }

      vector< pair< account_id_type, address > > tuples_from_index;
      tuples_from_index.reserve( tuples_from_db.size() );
      const auto& key_account_idx =
         db.get_index_type<graphene::account_history::key_account_index>()
         .indices().get<graphene::account_history::by_key>();

      for( const graphene::account_history::key_account_object& key_account : key_account_idx )
      {
         address addr = key_account.key;
         for( const account_id_type& account_id : key_account.account_ids )
            tuples_from_index.emplace_back( account_id, addr );
      }

      // TODO:  use function for common functionality
      {
         // due to hashed index, account_id's may not be in sorted order...
         std::sort( tuples_from_db.begin(), tuples_from_db.end() );
         size_t size_before_uniq = tuples_from_db.size();
         auto last = std::unique( tuples_from_db.begin(), tuples_from_db.end() );
         tuples_from_db.erase( last, tuples_from_db.end() );
         // but they should be unique (multiple instances of the same
         //  address within an account should have been de-duplicated
         //  by the flat_set above)
         BOOST_CHECK( tuples_from_db.size() == size_before_uniq );
      }

      {
         // (address, account) should be de-duplicated by flat_set<>
         // in key_account_object
         std::sort( tuples_from_index.begin(), tuples_from_index.end() );
         auto last = std::unique( tuples_from_index.begin(), tuples_from_index.end() );
         size_t size_before_uniq = tuples_from_db.size();
         tuples_from_index.erase( last, tuples_from_index.end() );
         BOOST_CHECK( tuples_from_index.size() == size_before_uniq );
      }

      //BOOST_CHECK_EQUAL( tuples_from_db, tuples_from_index );
      bool is_equal = true;
      is_equal &= (tuples_from_db.size() == tuples_from_index.size());
      for( size_t i=0,n=tuples_from_db.size(); i<n; i++ )
         is_equal &= (tuples_from_db[i] == tuples_from_index[i] );

      bool account_history_plugin_index_ok = is_equal;
      BOOST_CHECK( account_history_plugin_index_ok );
         */
   }
   return;
}

void database_fixture::open_database()
{
   if( !data_dir ) {
      data_dir = fc::temp_directory( graphene::utilities::temp_directory_path() );
      db.open(data_dir->path(), [this]{return genesis_state;}, "test");
   }
}

signed_block database_fixture::generate_block(uint32_t skip, const fc::ecc::private_key& key, int miss_blocks)
{
   skip |= database::skip_undo_history_check;
   // skip == ~0 will skip checks specified in database::validation_steps
   auto block = db.generate_block(db.get_slot_time(miss_blocks + 1),
                            db.get_scheduled_witness(miss_blocks + 1),
                            key, skip);
   db.clear_pending();
   return block;
}

void database_fixture::generate_blocks( uint32_t block_count )
{
   for( uint32_t i = 0; i < block_count; ++i )
      generate_block();
}

void database_fixture::generate_blocks(fc::time_point_sec timestamp, bool miss_intermediate_blocks, uint32_t skip)
{
   if( miss_intermediate_blocks )
   {
      generate_block(skip);
      auto slots_to_miss = db.get_slot_at_time(timestamp);
      if( slots_to_miss <= 1 )
         return;
      --slots_to_miss;
      generate_block(skip, init_account_priv_key, slots_to_miss);
      return;
   }
   while( db.head_block_time() < timestamp )
      generate_block(skip);
}

account_create_operation database_fixture::make_account_seed(
   uint32_t seed,
   const std::string& name /* = "nathan" */,
   public_key_type key /* = key_id_type() */
   )
   {
      auto uid = graphene::chain::calc_account_uid( seed );
      return make_account( uid, name, key );
   }

account_create_operation database_fixture::make_account(
   account_uid_type uid,
   const std::string& name /* = "nathan" */,
   public_key_type key /* = key_id_type() */
   )
{ try {
   account_reg_info reg;
   account_create_operation create_account;

   reg.allowance_per_article = asset(10000);
   reg.max_share_per_article = asset(5000);
   reg.max_share_total = asset(1000);
   reg.registrar = GRAPHENE_NULL_ACCOUNT_UID;
   
   create_account.uid = uid;
   create_account.name = name;
   create_account.owner = authority(1, key, 1);
   create_account.active = authority(1, key, 1);
   create_account.secondary = authority(1, key, 1);
   create_account.memo_key = key;
   create_account.reg_info = reg;

   create_account.fee = db.current_fee_schedule().calculate_fee( create_account );
   return create_account;
} FC_CAPTURE_AND_RETHROW() }

account_create_operation database_fixture::make_account(
   account_uid_type uid,
   const std::string& name,
   const account_object& registrar,
   const account_object& referrer,
   uint8_t referrer_percent /* = 100 */,
   public_key_type key /* = public_key_type() */
   )
{
   try
   {
      account_reg_info                  reg;
      account_create_operation          create_account;

      reg.registrar             = registrar.uid;
      reg.referrer              = referrer.uid;
      reg.referrer_percent      = referrer_percent;
      reg.allowance_per_article = asset(10000);
      reg.max_share_per_article = asset(5000);
      reg.max_share_total       = asset(1000);

      create_account.uid = uid;
      create_account.name = name;
      create_account.owner = authority(1, key, 1);
      create_account.active = authority(1, key, 1);
      create_account.secondary = authority(1, key, 1);
      create_account.memo_key = key;
      create_account.reg_info = reg;

      create_account.fee = db.current_fee_schedule().calculate_fee( create_account );
      return create_account;
   }
   FC_CAPTURE_AND_RETHROW((name)(referrer_percent))
}

const asset_object& database_fixture::get_asset( const string& symbol )const
{
   const auto& idx = db.get_index_type<asset_index>().indices().get<by_symbol>();
   const auto itr = idx.find(symbol);
   assert( itr != idx.end() );
   return *itr;
}

const account_object& database_fixture::get_account( const string& name )const
{
   const auto& idx = db.get_index_type<account_index>().indices().get<by_name>();
   const auto itr = idx.find(name);
   assert( itr != idx.end() );
   return *itr;
}

const asset_object& database_fixture::create_user_issued_asset( const string& name )
{
   asset_create_operation creator;
   creator.issuer = GRAPHENE_COMMITTEE_ACCOUNT_UID;
   creator.fee = asset();
   creator.symbol = name;
   creator.common_options.max_supply = 0;
   creator.precision = 2;
   creator.common_options.max_supply = GRAPHENE_MAX_SHARE_SUPPLY;
   //creator.common_options.flags = charge_market_fee;
   //creator.common_options.issuer_permissions = charge_market_fee;
   trx.operations.push_back(std::move(creator));
   trx.validate();
   processed_transaction ptx = db.push_transaction(trx, ~0);
   trx.operations.clear();
   return db.get<asset_object>(ptx.operation_results[0].get<object_id_type>());
}

const asset_object& database_fixture::create_user_issued_asset( const string& name, const account_object& issuer, uint16_t flags )
{
   asset_create_operation creator;
   creator.issuer = issuer.uid;
   creator.fee = asset();
   creator.symbol = name;
   creator.common_options.max_supply = 0;
   creator.precision = 2;
   creator.common_options.max_supply = GRAPHENE_MAX_SHARE_SUPPLY;
   creator.common_options.flags = flags;
   creator.common_options.issuer_permissions = flags;
   trx.operations.clear();
   trx.operations.push_back(std::move(creator));
   set_expiration( db, trx );
   trx.validate();
   processed_transaction ptx = db.push_transaction(trx, ~0);
   trx.operations.clear();
   return db.get<asset_object>(ptx.operation_results[0].get<object_id_type>());
}

void database_fixture::issue_uia( const account_object& recipient, asset amount )
{
   BOOST_TEST_MESSAGE( "Issuing UIA" );
   asset_issue_operation op;
   op.issuer = db.get_asset_by_aid( amount.asset_id ).issuer;
   op.asset_to_issue = amount;
   op.issue_to_account = recipient.uid;
   trx.operations.push_back(op);
   db.push_transaction( trx, ~0 );
   trx.operations.clear();
}

void database_fixture::issue_uia( account_uid_type recipient_id, asset amount )
{
   issue_uia( db.get_account_by_uid( recipient_id ), amount );
}

void database_fixture::change_fees(
   const flat_set< fee_parameters >& new_params,
   uint32_t new_scale /* = 0 */
   )
{
   const chain_parameters& current_chain_params = db.get_global_properties().parameters;
   const fee_schedule& current_fees = *(current_chain_params.current_fees);

   flat_map< int, fee_parameters > fee_map;
   fee_map.reserve( current_fees.parameters.size() );
   for( const fee_parameters& op_fee : current_fees.parameters )
      fee_map[ op_fee.which() ] = op_fee;
   for( const fee_parameters& new_fee : new_params )
      fee_map[ new_fee.which() ] = new_fee;

   fee_schedule_type new_fees;

   for( const std::pair< int, fee_parameters >& item : fee_map )
      new_fees.parameters.insert( item.second );
   if( new_scale != 0 )
      new_fees.scale = new_scale;

   chain_parameters new_chain_params = current_chain_params;
   new_chain_params.current_fees = new_fees;

   db.modify(db.get_global_properties(), [&](global_property_object& p) {
      p.parameters = new_chain_params;
   });
}

const account_object& database_fixture::create_account(
   const uint32_t seed,
   const string& name,
   const public_key_type& key /* = public_key_type() */
   )
{
   auto uid = graphene::chain::calc_account_uid( seed );
   return create_account( uid, name, key );
}

const account_object& database_fixture::create_account(
   const account_uid_type uid,
   const string& name,
   const public_key_type& key /* = public_key_type() */
   )
{
   trx.operations.push_back(make_account(uid, name, key));
   trx.validate();
   processed_transaction ptx = db.push_transaction(trx, ~0);
   auto& result = db.get<account_object>(ptx.operation_results[0].get<object_id_type>());
   trx.operations.clear();
   return result;
}

const account_object& database_fixture::create_account(
   const account_uid_type uid,
   const string& name,
   const account_object& registrar,
   const account_object& referrer,
   uint8_t referrer_percent /* = 100 */,
   const public_key_type& key /*= public_key_type()*/
   )
{
   try
   {
      trx.operations.resize(1);
      trx.operations.back() = (make_account(uid, name, registrar, referrer, referrer_percent, key));
      trx.validate();
      auto r = db.push_transaction(trx, ~0);
      const auto& result = db.get<account_object>(r.operation_results[0].get<object_id_type>());
      trx.operations.clear();
      return result;
   }
   FC_CAPTURE_AND_RETHROW( (name)(registrar)(referrer) )
}

const account_object& database_fixture::create_account(
   const account_uid_type uid,
   const string& name,
   const private_key_type& key,
   const account_uid_type& registrar_id /* = account_id_type() */,
   const account_uid_type& referrer_id /* = account_id_type() */,
   uint8_t referrer_percent /* = 100 */
   )
{
   try
   {
      trx.operations.clear();

      account_reg_info reg;
      account_create_operation account_create_op;

      reg.registrar             = registrar_id;
      reg.referrer              = referrer_id;
      reg.allowance_per_article = asset(10000);
      reg.max_share_per_article = asset(5000);
      reg.max_share_total       = asset(1000);

      account_create_op.uid = uid;
      account_create_op.name = name;
      account_create_op.owner = authority(123, public_key_type(key.get_public_key()), 123);
      account_create_op.active = authority(456, public_key_type(key.get_public_key()), 456);
      account_create_op.secondary = authority(789, public_key_type(key.get_public_key()), 789);
      account_create_op.memo_key = key.get_public_key();
      account_create_op.reg_info = reg;
      trx.operations.push_back( account_create_op );

      trx.validate();

      processed_transaction ptx = db.push_transaction(trx, ~0);
      //wdump( (ptx) );
      const account_object& result = db.get<account_object>(ptx.operation_results[0].get<object_id_type>());
      trx.operations.clear();
      return result;
   }
   FC_CAPTURE_AND_RETHROW( (name)(registrar_id)(referrer_id) )
}

const committee_member_object& database_fixture::create_committee_member( const account_object& owner )
{
   committee_member_create_operation op;
   op.account = owner.uid;
   trx.operations.push_back(op);
   trx.validate();
   processed_transaction ptx = db.push_transaction(trx, ~0);
   trx.operations.clear();
   return db.get<committee_member_object>(ptx.operation_results[0].get<object_id_type>());
}

const witness_object&database_fixture::create_witness(account_uid_type owner, const fc::ecc::private_key& signing_private_key)
{
   return create_witness(db.get_account_by_uid( owner ), signing_private_key);
}

const witness_object& database_fixture::create_witness( const account_object& owner,
                                                        const fc::ecc::private_key& signing_private_key )
{ try {
   witness_create_operation op;
   op.account = owner.uid;
   op.block_signing_key = signing_private_key.get_public_key();
   op.pledge = asset(10000);
   op.url = "";
   trx.operations.push_back(op);
   trx.validate();
   processed_transaction ptx = db.push_transaction(trx, ~0);
   trx.clear();
   return db.get<witness_object>(ptx.operation_results[0].get<object_id_type>());
} FC_CAPTURE_AND_RETHROW() }

//const worker_object& database_fixture::create_worker( const account_id_type owner, const share_type daily_pay, const fc::microseconds& duration )
//{ try {
//   worker_create_operation op;
//   op.owner = owner;
//   op.daily_pay = daily_pay;
//   op.initializer = burn_worker_initializer();
//   op.work_begin_date = db.head_block_time();
//   op.work_end_date = op.work_begin_date + duration;
//   trx.operations.push_back(op);
//   trx.validate();
//   processed_transaction ptx = db.push_transaction(trx, ~0);
//   trx.clear();
//   return db.get<worker_object>(ptx.operation_results[0].get<object_id_type>());
//} FC_CAPTURE_AND_RETHROW() }

uint64_t database_fixture::fund(
   const account_object& account,
   const asset& amount /* = asset(500000) */
   )
{
   transfer(db.get_account_by_uid( GRAPHENE_NULL_ACCOUNT_UID ), account, amount);
   return get_balance( account.uid, amount.asset_id );
}

void database_fixture::sign(signed_transaction& trx, const fc::ecc::private_key& key)
{
   trx.sign( key, db.get_chain_id() );
}

digest_type database_fixture::digest( const transaction& tx )
{
   return tx.digest();
}

//const limit_order_object*database_fixture::create_sell_order(account_uid_type user, const asset& amount, const asset& recv)
//{
//   //TODO: market
//   //auto r =  create_sell_order(user(db), amount, recv);
//   //verify_asset_supplies(db);
//   //return r;
//
//   return nullptr;
//}
//
//const limit_order_object* database_fixture::create_sell_order( const account_object& user, const asset& amount, const asset& recv )
//{
//   //TODO: market
//   /*
//   limit_order_create_operation buy_order;
//   buy_order.seller = user.id;
//   buy_order.amount_to_sell = amount;
//   buy_order.min_to_receive = recv;
//   trx.operations.push_back(buy_order);
//   for( auto& op : trx.operations ) db.current_fee_schedule().set_fee(op);
//   trx.validate();
//   auto processed = db.push_transaction(trx, ~0);
//   trx.operations.clear();
//   verify_asset_supplies(db);
//   //wdump((processed));
//   return db.find<limit_order_object>( processed.operation_results[0].get<object_id_type>() );
//   */
//   return nullptr;
//}
//
//asset database_fixture::cancel_limit_order( const limit_order_object& order )
//{
//  //TODO: market
//  /*
//  limit_order_cancel_operation cancel_order;
//  cancel_order.fee_paying_account = order.seller;
//  cancel_order.order = order.id;
//  trx.operations.push_back(cancel_order);
//  for( auto& op : trx.operations ) db.current_fee_schedule().set_fee(op);
//  trx.validate();
//  auto processed = db.push_transaction(trx, ~0);
//  trx.operations.clear();
//   verify_asset_supplies(db);
//  return processed.operation_results[0].get<asset>();
//  */
//  return asset();
//}

void database_fixture::transfer(
   account_uid_type from,
   account_uid_type to,
   const asset& amount,
   const asset& fee /* = asset() */
   )
{
   transfer(db.get_account_by_uid( from ), db.get_account_by_uid( to ), amount, fee);
}

void database_fixture::transfer(
   const account_object& from,
   const account_object& to,
   const asset& amount,
   const asset& fee /* = asset() */ )
{
   try
   {
      set_expiration( db, trx );
      transfer_operation trans;
      trans.from = from.uid;
      trans.to   = to.uid;
      trans.amount = amount;
      trx.operations.push_back(trans);

      if( fee == asset() )
      {
         for( auto& op : trx.operations ) db.current_fee_schedule().set_fee(op);
      }
      trx.validate();
      db.push_transaction(trx, ~0);
      //verify_asset_supplies(db);
      trx.operations.clear();
   } FC_CAPTURE_AND_RETHROW( (from.id)(to.id)(amount)(fee) )
}

void database_fixture::enable_fees()
{
   db.modify(global_property_id_type()(db), [](global_property_object& gpo)
   {
      gpo.parameters.current_fees = fee_schedule::get_default();
   });
}

void database_fixture::upgrade_to_lifetime_member(account_uid_type account)
{
   //TODO:lifetime member
   //upgrade_to_lifetime_member(account(db));
}

void database_fixture::upgrade_to_lifetime_member( const account_object& account )
{
   //TODO:lifetime member
   /*
   try
   {
      account_upgrade_operation op;
      op.account_to_upgrade = account.get_id();
      op.upgrade_to_lifetime_member = true;
      op.fee = db.get_global_properties().parameters.current_fees->calculate_fee(op);
      trx.operations = {op};
      db.push_transaction(trx, ~0);
      FC_ASSERT( op.account_to_upgrade(db).is_lifetime_member() );
      trx.clear();
      verify_asset_supplies(db);
   }
   FC_CAPTURE_AND_RETHROW((account))
   */
}

void database_fixture::upgrade_to_annual_member(account_uid_type account)
{
   //TODO:annual member
   //upgrade_to_annual_member(account(db));
}

void database_fixture::upgrade_to_annual_member(const account_object& account)
{
   //TODO:annual member
   /*
   try {
      account_upgrade_operation op;
      op.account_to_upgrade = account.get_id();
      op.fee = db.get_global_properties().parameters.current_fees->calculate_fee(op);
      trx.operations = {op};
      db.push_transaction(trx, ~0);
      FC_ASSERT( op.account_to_upgrade(db).is_member(db.head_block_time()) );
      trx.clear();
      verify_asset_supplies(db);
   } FC_CAPTURE_AND_RETHROW((account))
   */
}

void database_fixture::print_market( const string& syma, const string& symb )const
{
   // TODO: market
   /*
   const auto& limit_idx = db.get_index_type<limit_order_index>();
   const auto& price_idx = limit_idx.indices().get<by_price>();

   cerr << std::fixed;
   cerr.precision(5);
   cerr << std::setw(10) << std::left  << "NAME"      << " ";
   cerr << std::setw(16) << std::right << "FOR SALE"  << " ";
   cerr << std::setw(16) << std::right << "FOR WHAT"  << " ";
   cerr << std::setw(10) << std::right << "PRICE (S/W)"   << " ";
   cerr << std::setw(10) << std::right << "1/PRICE (W/S)" << "\n";
   cerr << string(70, '=') << std::endl;
   auto cur = price_idx.begin();
   while( cur != price_idx.end() )
   {
      cerr << std::setw( 10 ) << std::left   << cur->seller(db).name << " ";
      cerr << std::setw( 10 ) << std::right  << cur->for_sale.value << " ";
      cerr << std::setw( 5 )  << std::left   << cur->amount_for_sale().asset_id(db).symbol << " ";
      cerr << std::setw( 10 ) << std::right  << cur->amount_to_receive().amount.value << " ";
      cerr << std::setw( 5 )  << std::left   << cur->amount_to_receive().asset_id(db).symbol << " ";
      cerr << std::setw( 10 ) << std::right  << cur->sell_price.to_real() << " ";
      cerr << std::setw( 10 ) << std::right  << (~cur->sell_price).to_real() << " ";
      cerr << "\n";
      ++cur;
   }
   */
}

string database_fixture::pretty( const asset& a )const
{
  std::stringstream ss;
  ss << a.amount.value << " ";
  ss << db.get_asset_by_aid( a.asset_id ).symbol;
  return ss.str();
}

//void database_fixture::print_limit_order( const limit_order_object& cur )const
//{
//  // TODO: market
//  /*
//  std::cout << std::setw(10) << cur.seller(db).name << " ";
//  std::cout << std::setw(10) << "LIMIT" << " ";
//  std::cout << std::setw(16) << pretty( cur.amount_for_sale() ) << " ";
//  std::cout << std::setw(16) << pretty( cur.amount_to_receive() ) << " ";
//  std::cout << std::setw(16) << cur.sell_price.to_real() << " ";
//  */
//}

void database_fixture::print_joint_market( const string& syma, const string& symb )const
{
  // TODO: market
  /*
  cout << std::fixed;
  cout.precision(5);

  cout << std::setw(10) << std::left  << "NAME"      << " ";
  cout << std::setw(10) << std::right << "TYPE"      << " ";
  cout << std::setw(16) << std::right << "FOR SALE"  << " ";
  cout << std::setw(16) << std::right << "FOR WHAT"  << " ";
  cout << std::setw(16) << std::right << "PRICE (S/W)" << "\n";
  cout << string(70, '=');

  const auto& limit_idx = db.get_index_type<limit_order_index>();
  const auto& limit_price_idx = limit_idx.indices().get<by_price>();

  auto limit_itr = limit_price_idx.begin();
  while( limit_itr != limit_price_idx.end() )
  {
     std::cout << std::endl;
     print_limit_order( *limit_itr );
     ++limit_itr;
  }
  */
}

int64_t database_fixture::get_balance( account_uid_type account, asset_aid_type a )const
{
  return db.get_balance( account, a ).amount.value;
}

int64_t database_fixture::get_balance( const account_object& account, const asset_object& a )const
{
  return db.get_balance( account.uid, a.asset_id ).amount.value;
}

vector< operation_history_object > database_fixture::get_operation_history( account_uid_type account_id )const
{
   vector< operation_history_object > result;
   const auto& stats = db.get_account_by_uid( account_id ).statistics(db);
   if(stats.most_recent_op == account_transaction_history_id_type())
      return result;

   const account_transaction_history_object* node = &stats.most_recent_op(db);
   while( true )
   {
      result.push_back( node->operation_id(db) );
      if(node->next == account_transaction_history_id_type())
         break;
      node = db.find(node->next);
   }
   return result;
}

void database_fixture::add_csaf_for_account(account_uid_type account, share_type csaf)
{
   db.modify(db.get_account_statistics_by_uid(account), [&](account_statistics_object& s) {
      s.csaf += csaf * 100000;
   });
}

//void database_fixture::collect_csaf(account_uid_type from, account_uid_type to, uint32_t amount, string asset_symbol)
//{
//   try
//   {
//      time_point_sec time((time_point::now().sec_since_epoch()) / 60 * 60);
//
//      asset_object asset_obj = get_asset(asset_symbol);
//      //FC_ASSERT(asset_obj, "Could not find asset matching ${asset}", ("asset", asset_symbol));
//
//
//      set_expiration(db, trx);
//      csaf_collect_operation cc_op;
//      cc_op.from = from;
//      cc_op.to = to;
//      cc_op.amount = asset(amount, asset_obj.asset_id);
//      cc_op.time = time;
//      auto a = fc::string(db.head_block_time());
//      auto b = fc::string(time);
//
//
//      cc_op.fee = fee_type(asset(100000));
//      trx.operations.push_back(cc_op);
//      //set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
//
//      trx.validate();
//      db.push_transaction(trx, ~0);
//      trx.operations.clear();
//   }FC_CAPTURE_AND_RETHROW((from)(to)(amount)(asset_symbol))
//}

void database_fixture::committee_proposal_create(
   const account_uid_type committee_member_account,
   const vector<committee_proposal_item_type> items,
   const uint32_t voting_closing_block_num,
   optional<voting_opinion_type> proposer_opinion,
   const uint32_t execution_block_num,
   const uint32_t expiration_block_num
   )
{
   try{
      committee_proposal_create_operation op;
      op.proposer = committee_member_account;
      op.items = items;
      op.voting_closing_block_num = voting_closing_block_num;
      op.proposer_opinion = proposer_opinion;
      op.execution_block_num = execution_block_num;
      op.expiration_block_num = expiration_block_num;

      signed_transaction tx;
      tx.operations.push_back(op);
      set_operation_fees(tx, db.current_fee_schedule());
      set_expiration(db, tx);
      tx.validate();

      sign(tx, init_account_priv_key);

      db.push_transaction(tx);
   }FC_CAPTURE_AND_RETHROW((committee_member_account)(items)(voting_closing_block_num)(proposer_opinion)(execution_block_num)(expiration_block_num))
   
}

void database_fixture::committee_proposal_vote(
   const account_uid_type committee_member_account,
   const uint64_t proposal_number,
   const voting_opinion_type opinion
   )
{
   try{
      committee_proposal_update_operation update_op;
      update_op.account = committee_member_account;
      update_op.proposal_number = proposal_number;
      update_op.opinion = opinion;

      signed_transaction tx;
      tx.operations.push_back(update_op);
      set_operation_fees(tx, db.current_fee_schedule());
      set_expiration(db, tx);
      tx.validate();

      sign(tx, init_account_priv_key);

      db.push_transaction(tx);
   }FC_CAPTURE_AND_RETHROW((committee_member_account)(proposal_number)(opinion))
}

void database_fixture::create_platform(account_uid_type owner_account,
   string name,
   asset pledge_amount,
   string url,
   string extra_data,
   flat_set<fc::ecc::private_key> sign_keys)
{
   try {
      platform_create_operation platform_create_op;
      platform_create_op.account = owner_account;
      platform_create_op.name = name;
      platform_create_op.pledge = pledge_amount;
      platform_create_op.extra_data = extra_data;
      platform_create_op.url = url;

      signed_transaction tx;
      tx.operations.push_back(platform_create_op);
      set_operation_fees(tx, db.current_fee_schedule());
      set_expiration(db, tx);
      tx.validate();

      for (auto key : sign_keys)
         sign(tx, key);

      db.push_transaction(tx);
   } FC_CAPTURE_AND_RETHROW((owner_account)(name)(pledge_amount)(url)(extra_data)(sign_keys))
}

void database_fixture::update_platform_votes(account_uid_type voting_account,
   flat_set<account_uid_type> platforms_to_add,
   flat_set<account_uid_type> platforms_to_remove,
   flat_set<fc::ecc::private_key> sign_keys
   )
{
   try {
      flat_set<account_uid_type> uids_to_add;
      flat_set<account_uid_type> uids_to_remove;
      uids_to_add.reserve(platforms_to_add.size());
      uids_to_remove.reserve(platforms_to_remove.size());
      for (auto pla : platforms_to_add)
         uids_to_add.insert(pla);
      for (auto pla : platforms_to_remove)
         uids_to_remove.insert(pla);

      platform_vote_update_operation platform_vote_update_op;
      platform_vote_update_op.voter = voting_account;
      platform_vote_update_op.platform_to_add = uids_to_add;
      platform_vote_update_op.platform_to_remove = uids_to_remove;

      signed_transaction tx;
      tx.operations.push_back(platform_vote_update_op);
      set_operation_fees(tx, db.current_fee_schedule());
      set_expiration(db, tx);
      tx.validate();

      for (auto key : sign_keys)
         sign(tx, key);

      db.push_transaction(tx);

   } FC_CAPTURE_AND_RETHROW((voting_account)(platforms_to_add)(platforms_to_remove)(sign_keys))
}

void database_fixture::reward_post(account_uid_type from_account,
   account_uid_type platform,
   account_uid_type poster,
   post_pid_type post_pid,
   asset amount,
   flat_set<fc::ecc::private_key> sign_keys)
{
   try {
      reward_operation reward_op;
      reward_op.from_account_uid = from_account;
      reward_op.platform = platform;
      reward_op.poster = poster;
      reward_op.post_pid = post_pid;
      reward_op.amount = amount;

      signed_transaction tx;
      tx.operations.push_back(reward_op);
      set_operation_fees(tx, db.current_fee_schedule());
      set_expiration(db, tx);
      tx.validate();

      for (auto key : sign_keys)
         sign(tx, key);

      db.push_transaction(tx);

   } FC_CAPTURE_AND_RETHROW((from_account)(platform)(poster)(post_pid)(amount)(sign_keys))
}

void database_fixture::reward_post_proxy_by_platform(account_uid_type from_account,
   account_uid_type platform,
   account_uid_type poster,
   post_pid_type    post_pid,
   uint64_t         amount,
   flat_set<fc::ecc::private_key> sign_keys)
{
   try {
      reward_proxy_operation reward_op;
      reward_op.from_account_uid = from_account;
      reward_op.platform = platform;
      reward_op.poster = poster;
      reward_op.post_pid = post_pid;
      reward_op.amount = amount; //must be YOYO

      signed_transaction tx;
      tx.operations.push_back(reward_op);
      set_operation_fees(tx, db.current_fee_schedule());
      set_expiration(db, tx);
      tx.validate();

      for (auto key : sign_keys)
         sign(tx, key);

      db.push_transaction(tx);
   } FC_CAPTURE_AND_RETHROW((from_account)(platform)(poster)(post_pid)(amount)(sign_keys))
}

void database_fixture::buyout_post(account_uid_type from_account,
   account_uid_type platform,
   account_uid_type poster,
   post_pid_type    post_pid,
   account_uid_type receiptor_account,
   flat_set<fc::ecc::private_key> sign_keys)
{
   try {
      buyout_operation buyout_op;
      buyout_op.from_account_uid = from_account;
      buyout_op.platform = platform;
      buyout_op.poster = poster;
      buyout_op.post_pid = post_pid;
      buyout_op.receiptor_account_uid = receiptor_account;

      signed_transaction tx;
      tx.operations.push_back(buyout_op);
      set_operation_fees(tx, db.current_fee_schedule());
      set_expiration(db, tx);
      tx.validate();

      for (auto key : sign_keys)
         sign(tx, key);

      db.push_transaction(tx);
   } FC_CAPTURE_AND_RETHROW((from_account)(platform)(poster)(post_pid)(receiptor_account)(sign_keys))
}

void database_fixture::create_license(account_uid_type platform,
    uint8_t license_type,
    string  hash_value,
    string  title,
    string  body,
    string  extra_data,
    flat_set<fc::ecc::private_key> sign_keys)
{
    try {
        const account_statistics_object& plat_account_statistics = db.get_account_statistics_by_uid(platform);
        license_create_operation create_op;
        create_op.license_lid = plat_account_statistics.last_license_sequence + 1;
        create_op.platform = platform;
        create_op.type = license_type;
        create_op.hash_value = hash_value;
        create_op.extra_data = extra_data;
        create_op.title = title;
        create_op.body = body;

        signed_transaction tx;
        tx.operations.push_back(create_op);
        set_operation_fees(tx, db.current_fee_schedule());
        set_expiration(db, tx);
        tx.validate();

        for (auto key : sign_keys)
           sign(tx, key);

        db.push_transaction(tx);
    } FC_CAPTURE_AND_RETHROW((platform)(license_type)(hash_value)(title)(body)(extra_data)(sign_keys))
}

void database_fixture::transfer_extension(flat_set<fc::ecc::private_key> sign_keys,
                                          account_uid_type from,
                                          account_uid_type to,
                                          asset            amount,
                                          string           memo,
                                          bool             isfrom_balance,
                                          bool             isto_balance)
{
    try{
        transfer_operation xfer_op;
        xfer_op.extensions = extension< transfer_operation::ext >();
        if (isfrom_balance)
            xfer_op.extensions->value.from_balance = amount.amount;
        else
            xfer_op.extensions->value.from_prepaid = amount.amount;
        if (isto_balance)
            xfer_op.extensions->value.to_balance = amount.amount;
        else
            xfer_op.extensions->value.to_prepaid = amount.amount;

        xfer_op.from = from;
        xfer_op.to = to;
        xfer_op.amount = amount.amount;

        if (memo.size())
        {
            account_object from_account = db.get_account_by_uid(from);
            account_object to_account = db.get_account_by_uid(to);
            xfer_op.memo = memo_data();
            xfer_op.memo->from = from_account.memo_key;
            xfer_op.memo->to = to_account.memo_key;
            //TODO
            /*xfer_op.memo->set_message(get_private_key(from_account.memo_key),
                to_account.memo_key, memo);*/
        }

        signed_transaction tx;
        tx.operations.push_back(xfer_op);
        set_operation_fees(tx, db.current_fee_schedule());
        set_expiration(db, tx);
        tx.validate();

        for (auto key : sign_keys)
            sign(tx, key);

        db.push_transaction(tx);
    }FC_CAPTURE_AND_RETHROW((sign_keys)(from)(to)(amount)(memo)(isfrom_balance)(isto_balance))
}

void database_fixture::account_auth_platform(flat_set<fc::ecc::private_key> sign_keys,
                                             account_uid_type account,
                                             account_uid_type platform_owner,
                                             share_type       limit_for_platform,
                                             uint32_t         permission_flags)
{
    try{
        account_auth_platform_operation op;
        op.uid = account;
        op.platform = platform_owner;

        account_auth_platform_operation::ext ext;
        ext.limit_for_platform = limit_for_platform;
        ext.permission_flags = permission_flags;
        op.extensions = flat_set<account_auth_platform_operation::extension_parameter>();
        op.extensions->insert(ext);

        signed_transaction tx;
        tx.operations.push_back(op);
        set_operation_fees(tx, db.current_fee_schedule());
        set_expiration(db, tx);
        tx.validate();

        for (auto key : sign_keys)
            sign(tx, key);

        db.push_transaction(tx);
    }FC_CAPTURE_AND_RETHROW((sign_keys)(account)(platform_owner)(limit_for_platform)(permission_flags))
}

void database_fixture::create_post(flat_set<fc::ecc::private_key> sign_keys,
                                   account_uid_type           platform,
                                   account_uid_type           poster,
                                   string                     hash_value,
                                   string                     title,
                                   string                     body,
                                   string                     extra_data,
                                   optional<account_uid_type> origin_platform,
                                   optional<account_uid_type> origin_poster,
                                   optional<post_pid_type>    origin_post_pid,
                                   post_operation::ext  exts)
{
    try{
        const account_statistics_object& poster_account_statistics = db.get_account_statistics_by_uid(poster);
        post_operation create_op;
        create_op.post_pid = poster_account_statistics.last_post_sequence + 1;
        create_op.platform = platform;
        create_op.poster = poster;
        if (origin_platform.valid())
            create_op.origin_platform = *origin_platform;
        if (origin_poster.valid())
            create_op.origin_poster = *origin_poster;
        if (origin_post_pid.valid())
            create_op.origin_post_pid = *origin_post_pid;

        create_op.hash_value = hash_value;
        create_op.extra_data = extra_data;
        create_op.title = title;
        create_op.body = body;

        create_op.extensions = flat_set<post_operation::extension_parameter>();
        create_op.extensions->insert(exts);

        signed_transaction tx;
        tx.operations.push_back(create_op);
        set_operation_fees(tx, db.current_fee_schedule());
        set_expiration(db, tx);
        tx.validate();

        for (auto key : sign_keys)
            sign(tx, key);

        db.push_transaction(tx);
    }FC_CAPTURE_AND_RETHROW((sign_keys)(platform)(poster)(hash_value)(title)(body)(extra_data)(origin_platform)(origin_poster)(origin_post_pid)(exts))
}

void database_fixture::update_post(flat_set<fc::ecc::private_key> sign_keys,
                                   account_uid_type              platform,
                                   account_uid_type              poster,
                                   post_pid_type                 post_pid,
                                   optional<string>              hash_value,
                                   optional<string>              title,
                                   optional<string>              body,
                                   optional<string>              extra_data,
                                   optional<post_update_operation::ext> ext)
{
    try{
        post_update_operation update_op;
        update_op.post_pid = post_pid;
        update_op.platform = platform;
        update_op.poster = poster;

        if (hash_value.valid())
            update_op.hash_value = *hash_value;
        if (extra_data.valid())
            update_op.extra_data = *extra_data;
        if (title.valid())
            update_op.title = *title;
        if (body.valid())
            update_op.body = *body;

        update_op.extensions = flat_set<post_update_operation::extension_parameter>();
        update_op.extensions->insert(*ext);

        signed_transaction tx;
        tx.operations.push_back(update_op);
        set_operation_fees(tx, db.current_fee_schedule());
        set_expiration(db, tx);
        tx.validate();

        for (auto key : sign_keys)
            sign(tx, key);

        db.push_transaction(tx);
    }FC_CAPTURE_AND_RETHROW((sign_keys)(platform)(poster)(post_pid)(hash_value)(title)(body)(extra_data)(ext))
}

void database_fixture::score_a_post(flat_set<fc::ecc::private_key> sign_keys,
                                    account_uid_type from_account,
                                    account_uid_type platform,
                                    account_uid_type poster,
                                    post_pid_type    post_pid,
                                    int8_t           score,
                                    share_type       csaf)
{
    try{
        score_create_operation create_op;
        create_op.from_account_uid = from_account;
        create_op.platform = platform;
        create_op.poster = poster;
        create_op.post_pid = post_pid;
        create_op.score = score;
        create_op.csaf = csaf;

        signed_transaction tx;
        tx.operations.push_back(create_op);
        set_operation_fees(tx, db.current_fee_schedule());
        set_expiration(db, tx);
        tx.validate();

        for (auto key : sign_keys)
            sign(tx, key);

        db.push_transaction(tx);
    }FC_CAPTURE_AND_RETHROW((sign_keys)(from_account)(platform)(poster)(post_pid)(score)(csaf))
}

void database_fixture::account_manage(account_uid_type account, account_manage_operation::opt options)
{
   db.modify(db.get_account_by_uid(account), [&](account_object& a){
      if (options.can_post.valid())
         a.can_post = *options.can_post;
      if (options.can_reply.valid())
         a.can_reply = *options.can_reply;
      if (options.can_rate.valid())
         a.can_rate = *options.can_rate;
      a.last_update_time = db.head_block_time();
   });
}

namespace test {

void set_expiration( const database& db, transaction& tx )
{
   const chain_parameters& params = db.get_global_properties().parameters;
   tx.set_reference_block(db.head_block_id());
   tx.set_expiration( db.head_block_time() + fc::seconds( params.block_interval * (params.maintenance_skip_slots + 1) * 3 ) );
   return;
}

bool _push_block( database& db, const signed_block& b, uint32_t skip_flags /* = 0 */ )
{
   return db.push_block( b, skip_flags);
}

processed_transaction _push_transaction( database& db, const signed_transaction& tx, uint32_t skip_flags /* = 0 */ )
{ try {
   auto pt = db.push_transaction( tx, skip_flags );
   database_fixture::verify_asset_supplies(db);
   return pt;
} FC_CAPTURE_AND_RETHROW((tx)) }

} // graphene::chain::test

} } // graphene::chain
