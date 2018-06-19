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

#include <bitset>
#include <iostream>

#include <boost/test/unit_test.hpp>

#include <graphene/chain/database.hpp>
#include <graphene/chain/protocol/protocol.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/proposal_object.hpp>
#include <graphene/chain/vesting_balance_object.hpp>

#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

using namespace graphene::chain;

BOOST_AUTO_TEST_SUITE(block_tests)

BOOST_FIXTURE_TEST_CASE( update_account_keys, database_fixture )
{
   try
   {
       account_uid_type sam_account_id = graphene::chain::calc_account_uid( 2018001 );
       account_uid_type alice_account_id = graphene::chain::calc_account_uid( 2018002 );

      const asset_object& core = db.get_core_asset();
      uint32_t skip_flags =
          database::skip_transaction_dupe_check
        | database::skip_witness_signature
        | database::skip_transaction_signatures
        | database::skip_authority_check
        ;

      // Sam is the creator of accounts
      private_key_type committee_key = init_account_priv_key;
      private_key_type sam_key = generate_private_key("sam");

      //
      // A = old key set
      // B = new key set
      //
      // we measure how many times we test following four cases:
      //
      //                                     A-B        B-A
      // alice     case_count[0]   A == B    empty      empty
      // bob       case_count[1]   A  < B    empty      nonempty
      // charlie   case_count[2]   B  < A    nonempty   empty
      // dan       case_count[3]   A nc B    nonempty   nonempty
      //
      // and assert that all four cases were tested at least once
      //
      account_object sam_account_object = create_account(sam_account_id "sam", sam_key );

      //Get a sane head block time
      generate_block( skip_flags );

      db.modify(db.get_global_properties(), [](global_property_object& p) {
         p.parameters.committee_proposal_review_period = fc::hours(1).to_seconds();
      });

      transaction tx;
      processed_transaction ptx;

      account_object committee_account_object = db.get_account_by_uid( committee_account );
      // transfer from committee account to Sam account
      transfer(committee_account_object, sam_account_object, core.amount(100000));

      const int num_keys = 5;
      vector< private_key_type > numbered_private_keys;
      vector< vector< public_key_type > > numbered_key_id;
      numbered_private_keys.reserve( num_keys );
      numbered_key_id.push_back( vector<public_key_type>() );
      numbered_key_id.push_back( vector<public_key_type>() );

      for( int i=0; i<num_keys; i++ )
      {
         private_key_type privkey = generate_private_key(
            std::string("key_") + std::to_string(i));
         public_key_type pubkey = privkey.get_public_key();
         address addr( pubkey );

         numbered_private_keys.push_back( privkey );
         numbered_key_id[0].push_back( pubkey );
         //numbered_key_id[1].push_back( addr );
      }

      // each element of possible_key_sched is a list of exactly num_keys
      // indices into numbered_key_id[use_address].  they are defined
      // by repeating selected elements of
      // numbered_private_keys given by a different selector.
      vector< vector< int > > possible_key_sched;
      const int num_key_sched = (1 << num_keys)-1;
      possible_key_sched.reserve( num_key_sched );

      for( int s=1; s<=num_key_sched; s++ )
      {
         vector< int > v;
         int i = 0;
         v.reserve( num_keys );
         while( v.size() < num_keys )
         {
            if( s & (1 << i) )
               v.push_back( i );
            i++;
            if( i >= num_keys )
               i = 0;
         }
         possible_key_sched.push_back( v );
      }

      // we can only undo in blocks
      generate_block( skip_flags );

      std::cout << "update_account_keys:  this test will take a few minutes...\n";
      for( int use_addresses=0; use_addresses<2; use_addresses++ )
      {
         vector< public_key_type > key_ids = numbered_key_id[ use_addresses ];
         for( int num_owner_keys=1; num_owner_keys<=2; num_owner_keys++ )
         {
            for( int num_active_keys=1; num_active_keys<=2; num_active_keys++ )
            {
               std::cout << use_addresses << num_owner_keys << num_active_keys << "\n";
               for( const vector< int >& key_sched_before : possible_key_sched )
               {
                  auto it = key_sched_before.begin();
                  vector< const private_key_type* > owner_privkey;
                  vector< const public_key_type* > owner_keyid;
                  owner_privkey.reserve( num_owner_keys );

                  trx.clear();
                  account_create_operation create_op;
                  create_op.uid = alice_account_id;
                  create_op.name = "alice";

                  for( int owner_index=0; owner_index<num_owner_keys; owner_index++ )
                  {
                     int i = *(it++);
                     create_op.owner.key_auths[ key_ids[ i ] ] = 1;
                     owner_privkey.push_back( &numbered_private_keys[i] );
                     owner_keyid.push_back( &key_ids[ i ] );
                  }
                  // size() < num_owner_keys is possible when some keys are duplicates
                  create_op.owner.weight_threshold = create_op.owner.key_auths.size();

                  for( int active_index=0; active_index<num_active_keys; active_index++ )
                     create_op.active.key_auths[ key_ids[ *(it++) ] ] = 1;
                  // size() < num_active_keys is possible when some keys are duplicates
                  create_op.active.weight_threshold = create_op.active.key_auths.size();

                  create_op.memo_key = key_ids[ *(it++) ] ;
                  

                  account_reg_info reg;

                  reg.allowance_per_article = asset(10000);
                  reg.max_share_per_article = asset(5000);
                  reg.max_share_total = asset(1000);
                  reg.registrar = committee_account;
                  create_op.reg_info = reg;

                  trx.operations.push_back( create_op );
                  // trx.sign( sam_key );
                  wdump( (trx) );

                  processed_transaction ptx_create = db.push_transaction( trx,
                     database::skip_transaction_dupe_check |
                     database::skip_transaction_signatures |
                     database::skip_authority_check
                      );

                  generate_block( skip_flags );
                  for( const vector< int >& key_sched_after : possible_key_sched )
                  {
                     auto it = key_sched_after.begin();

                     trx.clear();
                     account_update_operation update_op;
                     update_op.account = alice_account_id;
                     update_op.owner = authority();
                     update_op.active = authority();
                     update_op.secondary = authority();

                     for( int owner_index=0; owner_index<num_owner_keys; owner_index++ )
                        update_op.owner->key_auths[ key_ids[ *(it++) ] ] = 1;
                     // size() < num_owner_keys is possible when some keys are duplicates
                     update_op.owner->weight_threshold = update_op.owner->key_auths.size();
                     for( int active_index=0; active_index<num_active_keys; active_index++ )
                        update_op.active->key_auths[ key_ids[ *(it++) ] ] = 1;
                     // size() < num_active_keys is possible when some keys are duplicates
                     update_op.active->weight_threshold = update_op.active->key_auths.size();
                     FC_ASSERT( update_op.new_options.valid() );
                     update_op.memo_key = key_ids[ *(it++) ] ;

                     trx.operations.push_back( update_op );
                     for( int i=0; i<int(create_op.owner.weight_threshold); i++)
                     {
                        sign( trx, *owner_privkey[i] );
                        if( i < int(create_op.owner.weight_threshold-1) )
                        {
                           GRAPHENE_REQUIRE_THROW(db.push_transaction(trx), fc::exception);
                        }
                        else
                        {
                           db.push_transaction( trx,
                           database::skip_transaction_dupe_check |
                           database::skip_transaction_signatures );
                        }
                     }
                     verify_account_history_plugin_index();
                     generate_block( skip_flags );

                     verify_account_history_plugin_index();
                     db.pop_block();
                     verify_account_history_plugin_index();
                  }
                  db.pop_block();
                  verify_account_history_plugin_index();
               }
            }
         }
      }
   }
   catch( const fc::exception& e )
   {
      edump( (e.to_detail_string()) );
      throw;
   }
}

BOOST_FIXTURE_TEST_CASE( tapos_rollover, database_fixture )
{
   try
   {
      ACTORS((2018003)(2018004));

      BOOST_TEST_MESSAGE( "Give Alice some money" );
      transfer(committee_account, u_2018003_id, asset(10000));
      generate_block();

      BOOST_TEST_MESSAGE( "Generate up to block 0xFF00" );
      generate_blocks( 0xFF00 );
      signed_transaction xfer_tx;

      BOOST_TEST_MESSAGE( "Transfer money at/about 0xFF00" );
      transfer_operation xfer_op;
      xfer_op.from = u_2018003_id;
      xfer_op.to = u_2018004_id;
      xfer_op.amount = asset(1000);

      xfer_tx.operations.push_back( xfer_op );
      xfer_tx.set_expiration( db.head_block_time() + fc::seconds( 0x1000 * db.get_global_properties().parameters.block_interval ) );
      xfer_tx.set_reference_block( db.head_block_id() );

      sign( xfer_tx, alice_private_key );
      PUSH_TX( db, xfer_tx, 0 );
      generate_block();

      BOOST_TEST_MESSAGE( "Sign new tx's" );
      xfer_tx.set_expiration( db.head_block_time() + fc::seconds( 0x1000 * db.get_global_properties().parameters.block_interval ) );
      xfer_tx.set_reference_block( db.head_block_id() );
      xfer_tx.signatures.clear();
      sign( xfer_tx, alice_private_key );

      BOOST_TEST_MESSAGE( "Generate up to block 0x10010" );
      generate_blocks( 0x110 );

      BOOST_TEST_MESSAGE( "Transfer at/about block 0x10010 using reference block at/about 0xFF00" );
      PUSH_TX( db, xfer_tx, 0 );
      generate_block();
   }
   catch (fc::exception& e)
   {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_SUITE_END()
