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
#include <graphene/chain/database.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/utilities/tempdir.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/smart_ref_impl.hpp>

#include <boost/test/auto_unit_test.hpp>

using namespace graphene::chain;

void set_expiration( const database& db, transaction& tx )
{
   const chain_parameters& params = db.get_global_properties().parameters;
   tx.set_reference_block(db.head_block_id());
   tx.set_expiration( db.head_block_time() + fc::seconds( params.block_interval * (params.maintenance_skip_slots + 1) * 3 ) );
   return;
}

BOOST_AUTO_TEST_CASE( operation_sanity_check )
{
   try {
      operation op = account_create_operation();
      op.get<account_create_operation>().active.add_authority(account_id_type(), 123);
      op.get<account_create_operation>().secondary.add_authority(account_id_type(), 123);
      operation tmp = std::move(op);
      wdump((tmp.which()));
   } catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE( genesis_and_persistence_bench )
{
   try {
      const int reserved_accounts = 10;
      genesis_state_type genesis_state;
      genesis_state.initial_timestamp = time_point_sec( fc::time_point::now().sec_since_epoch()
                                                        / GRAPHENE_DEFAULT_BLOCK_INTERVAL
                                                        * GRAPHENE_DEFAULT_BLOCK_INTERVAL);
      genesis_state.initial_active_witnesses = 11;

      const int all_reserved = reserved_accounts + genesis_state.initial_active_witnesses;

#ifdef NDEBUG
      ilog("Running in release mode.");
      const int account_count = reserved_accounts + 2000000;
      const int blocks_to_produce = 1000000;
#else
      ilog("Running in debug mode.");
      const int account_count = reserved_accounts + 30000;
      const int blocks_to_produce = 10;
#endif

      auto to_uid = graphene::chain::calc_account_uid( account_count );
      

      fc::ecc::private_key init_account_priv_key = fc::ecc::private_key::regenerate(fc::sha256::hash(string("null_key")) );
      public_key_type init_account_pub_key = init_account_priv_key.get_public_key();
      for( int i = 0; i < genesis_state.initial_active_witnesses; i++ )
      {
            auto name = "init"+fc::to_string(i);
            genesis_state.initial_accounts.emplace_back(graphene::chain::calc_account_uid(i+reserved_accounts),
                                                      name,
                                                      0,
                                                      init_account_pub_key,
                                                      init_account_pub_key,
                                                      init_account_pub_key,
                                                      init_account_pub_key,
                                                      true);
            genesis_state.initial_committee_candidates.push_back({name});
            genesis_state.initial_witness_candidates.push_back({name, init_account_pub_key});
      }
      genesis_state.initial_parameters.current_fees->zero_all_fees();
      auto alloc_balance = 1000; //GRAPHENE_MAX_SHARE_SUPPLY / account_count
      for( int i = 0; i < account_count; i++ )
      {
          public_key_type pkt = public_key_type(fc::ecc::private_key::regenerate(fc::digest(i)).get_public_key());
          genesis_state.initial_accounts.emplace_back(graphene::chain::calc_account_uid(i + all_reserved),
                                                     "target"+fc::to_string(i),
                                                     0,
                                                     pkt,
                                                     pkt,
                                                     pkt,
                                                     pkt
                                                     );
          genesis_state.initial_account_balances.emplace_back(graphene::chain::calc_account_uid(i + all_reserved),
                                                             "YOYO",
                                                             alloc_balance);
      }

      fc::temp_directory data_dir( graphene::utilities::temp_directory_path() );

      {
         database db;
         db.open(data_dir.path(), [&]{return genesis_state;}, "test");

         for( int i = 0; i < account_count; i++)
            BOOST_CHECK(db.get_balance(graphene::chain::calc_account_uid(i + all_reserved), GRAPHENE_CORE_ASSET_AID).amount == alloc_balance);

         ilog("to balance ================== ${a}",("a",db.get_balance(to_uid, GRAPHENE_CORE_ASSET_AID).amount));
         fc::time_point start_time = fc::time_point::now();
         db.close();
         ilog("Closed database in ${t} milliseconds.", ("t", (fc::time_point::now() - start_time).count() / 1000));
      }
      
      {
         database db;

         fc::time_point start_time = fc::time_point::now();
         db.open(data_dir.path(), [&]{return genesis_state;}, "test");
         ilog("Opened database in ${t} milliseconds.", ("t", (fc::time_point::now() - start_time).count() / 1000));

         for( int i = 0; i < account_count; i++)
            BOOST_CHECK(db.get_balance(graphene::chain::calc_account_uid(i + all_reserved), GRAPHENE_CORE_ASSET_AID).amount == alloc_balance);

         int blocks_out = 0;
         auto witness_priv_key = fc::ecc::private_key::regenerate(fc::sha256::hash(string("null_key")) );
         auto aw = db.get_global_properties().active_witnesses;
         auto b =  db.generate_block( db.get_slot_time( 1 ), db.get_scheduled_witness( 1 ), witness_priv_key, ~0 );

         start_time = fc::time_point::now();

         ilog("before tr to_balance ================== ${a}",("a",db.get_balance(to_uid, GRAPHENE_CORE_ASSET_AID).amount));
         
         /* 
         TODO：review
         signed_transaction trx;
         for( int i = 0; i < blocks_to_produce; i++ )
         {
            set_expiration( db, trx );
            transfer_operation tr_op;
            tr_op.fee = asset(1);
            tr_op.from = graphene::chain::calc_account_uid(i + all_reserved );
            tr_op.to = to_uid;
            tr_op.amount = asset(1);
            trx.operations.push_back(tr_op);
            //trx.expiration = time_point_sec(fc::time_point::now().sec_since_epoch() + 43200);
            
            trx.validate();
            db.push_transaction(trx, ~0);
            trx.operations.clear();

            aw = db.get_global_properties().active_witnesses;
            b =  db.generate_block( db.get_slot_time( 1 ), db.get_scheduled_witness( 1 ), witness_priv_key, ~0 );
         }
         */
         
         ilog("Pushed ${c} blocks (1 op each, no validation) in ${t} milliseconds.",
              ("c", blocks_out)("t", (fc::time_point::now() - start_time).count() / 1000));

         ilog("after tr to_balance ================== ${a}",("a",db.get_balance(to_uid, GRAPHENE_CORE_ASSET_AID).amount));

         start_time = fc::time_point::now();
         db.close();
         ilog("Closed database in ${t} milliseconds.", ("t", (fc::time_point::now() - start_time).count() / 1000));
      }
      {
         database db;

         auto start_time = fc::time_point::now();
         wlog( "about to start reindex..." );
         db.open(data_dir.path(), [&]{return genesis_state;}, "force_wipe");
         ilog("Replayed database in ${t} milliseconds.", ("t", (fc::time_point::now() - start_time).count() / 1000));

         auto acs = alloc_balance;// - 2;
         for( int i = 0; i < blocks_to_produce; i++ )
         {
               auto m_uid = graphene::chain::calc_account_uid(i + all_reserved );
               auto ams = db.get_balance(m_uid, GRAPHENE_CORE_ASSET_AID).amount;
               ilog("uid = ${uid};amount = ${a}; acs = ${b}",("uid",m_uid)("a",ams)("b",acs));      
               BOOST_CHECK( ams == acs);
         }
         ilog("to balance =============== ${a}",("a",db.get_balance(to_uid, GRAPHENE_CORE_ASSET_AID).amount));
            
      }

   } catch(fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}
