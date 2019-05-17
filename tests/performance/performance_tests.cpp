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

#include <graphene/chain/database.hpp>
#include <graphene/chain/protocol/protocol.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/proposal_object.hpp>

#include <graphene/db/simple_index.hpp>

#include <fc/crypto/digest.hpp>
#include "../common/database_fixture.hpp"

using namespace graphene::chain;

BOOST_FIXTURE_TEST_SUITE( performance_tests, database_fixture )

BOOST_AUTO_TEST_CASE( sigcheck_benchmark )
{
   fc::ecc::private_key nathan_key = fc::ecc::private_key::generate();
   auto digest = fc::sha256::hash("hello");
   auto sig = nathan_key.sign_compact( digest );
   auto start = fc::time_point::now();
   for( uint32_t i = 0; i < 100000; ++i )
      auto pub = fc::ecc::public_key( sig, digest );
   auto end = fc::time_point::now();
   auto elapsed = end-start;
   wdump( ((100000.0*1000000.0) / elapsed.count()) );
}

BOOST_AUTO_TEST_CASE(post_performance_test)
{
    ACTORS((1000)(9000));
    const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
    auto _core = [&](int64_t x) -> asset
    {  return asset(x*prec);    };
    transfer(committee_account, u_9000_id, _core(10000));
    add_csaf_for_account(u_9000_id, 10000);

    // post test
    flat_set<fc::ecc::private_key> sign_keys;
    sign_keys.insert(u_9000_private_key);
    create_platform(u_9000_id, "platform", _core(10000), "www.123456789.com", "", sign_keys);
    create_license(u_9000_id, 6, "999999999", "license title", "license body", "extra", sign_keys);

    flat_set<fc::ecc::private_key> sign_keys1;
    sign_keys1.insert(u_1000_private_key);
    account_auth_platform(sign_keys1, u_1000_id, u_9000_id, 1000 * prec, account_auth_platform_object::Platform_Permission_Forward |
                                                                         account_auth_platform_object::Platform_Permission_Liked |
                                                                         account_auth_platform_object::Platform_Permission_Buyout |
                                                                         account_auth_platform_object::Platform_Permission_Comment |
                                                                         account_auth_platform_object::Platform_Permission_Reward |
                                                                         account_auth_platform_object::Platform_Permission_Post |
                                                                         account_auth_platform_object::Platform_Permission_Content_Update);

    const uint64_t cycles = 200000;
    uint64_t total_time = 0;
    std::vector<account_id_type> accounts;
    accounts.reserve(cycles + 1);
    std::vector<asset_id_type> assets;
    assets.reserve(cycles);
    std::vector<signed_transaction> transactions;
    transactions.reserve(cycles);

    map<account_uid_type, Receiptor_Parameter> receiptors;
    receiptors.insert(std::make_pair(u_9000_id, Receiptor_Parameter{ GRAPHENE_DEFAULT_PLATFORM_RECEIPTS_RATIO, false, 0, 0 }));
    receiptors.insert(std::make_pair(u_1000_id, Receiptor_Parameter{ 7500, false, 0, 0 }));
    post_operation::ext extension;
    extension.post_type = post_operation::Post_Type_Post;
    extension.forward_price = 10000*prec;
    extension.receiptors = receiptors;
    extension.license_lid = 1;
    extension.permission_flags = post_object::Post_Permission_Forward |
                                 post_object::Post_Permission_Liked |
                                 post_object::Post_Permission_Buyout |
                                 post_object::Post_Permission_Comment |
                                 post_object::Post_Permission_Reward;

    const account_statistics_object& poster_account_statistics = db.get_account_statistics_by_uid(u_1000_id);
    post_operation create_op;
    create_op.post_pid = poster_account_statistics.last_post_sequence + i;
    create_op.platform = u_9000_id;
    create_op.poster = u_1000_id;
    create_op.hash_value = "6666666";
    create_op.extra_data = "extra";
    create_op.title = "document name";
    create_op.body = "document body";
    create_op.extensions = graphene::chain::extension<post_operation::ext>();
    create_op.extensions->value = extension;

    for (uint32_t i = 0; i < cycles; ++i){
        signed_transaction tx;
        tx.operations.push_back(create_op);
        set_operation_fees(tx, db.current_fee_schedule());
        test::set_expiration(db, tx);
        tx.validate();
        transactions.push_back(tx);
        tx.operations.clear();
    }

    auto start = fc::time_point::now();
    for (uint32_t i = 0; i < cycles; ++i)
    {
        auto result = db.apply_transaction(transactions[i]);
    }
    auto end = fc::time_point::now();
    auto elapsed = end - start;
    total_time += elapsed.count();
    wlog("Create ${aps} post/s over ${total}ms",
        ("aps", (cycles * 1000000) / elapsed.count())("total", elapsed.count() / 1000));

    //comment post test

    const fc::ecc::private_key nathan_key = fc::ecc::private_key::generate();
    const fc::ecc::public_key  nathan_pub = nathan_key.get_public_key();

    
    uint64_t total_time = 0;
    uint64_t total_count = 0;
    std::vector<account_id_type> accounts;
    accounts.reserve(cycles + 1);
    std::vector<asset_id_type> assets;
    assets.reserve(cycles);

    std::vector<signed_transaction> transactions;
    transactions.reserve(cycles);

    //post test
    account_reg_info reginfo;

    account_create_operation aco;
    aco.name = "a1";
    aco.reg_info = reginfo;
    aco.owner = authority(1, public_key_type(nathan_pub), 1);
    aco.active = authority(1, public_key_type(nathan_pub), 1);
    aco.secondary = authority(1, public_key_type(nathan_pub), 1);

    aco.fee = db.current_fee_schedule().calculate_fee(aco);
    //trx.clear();
    test::set_expiration(db, trx);
    for (uint32_t i = 0; i < cycles; ++i)
    {
        aco.name = "a" + fc::to_string(i);
        trx.operations.push_back(aco);
        transactions.push_back(trx);
        trx.operations.clear();
        ++total_count;
    }

    auto start = fc::time_point::now();
    for (uint32_t i = 0; i < cycles; ++i)
    {
        auto result = db.apply_transaction(transactions[i], ~0);
        accounts[i] = result.operation_results[0].get<object_id_type>();
    }
    auto end = fc::time_point::now();
    auto elapsed = end - start;
    total_time += elapsed.count();
    wlog("Create ${aps} accounts/s over ${total}ms",
        ("aps", (cycles * 1000000) / elapsed.count())("total", elapsed.count() / 1000));
}

BOOST_AUTO_TEST_CASE(post_performance_test_2)
{
   try{
      ACTORS((1000)(1001));
      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };
      transfer(committee_account, u_1001_id, _core(10000));
      add_csaf_for_account(u_1001_id, 10000);

      //create post
      flat_set<fc::ecc::private_key> sign_keys;
      sign_keys.insert(u_1001_private_key);
      create_platform(u_1001_id, "platform", _core(10000), "www.123456789.com", "", sign_keys);
      create_license(u_1001_id, 6, "999999999", "license title", "license body", "extra", sign_keys);

      flat_set<fc::ecc::private_key> sign_keys1;
      sign_keys1.insert(u_1000_private_key);
      account_auth_platform(sign_keys1, u_1000_id, u_1001_id, 1000 * prec, 255);

      map<account_uid_type, Receiptor_Parameter> receiptors;
      receiptors.insert(std::make_pair(u_1001_id, Receiptor_Parameter{ GRAPHENE_DEFAULT_PLATFORM_RECEIPTS_RATIO, false, 0, 0 }));
      receiptors.insert(std::make_pair(u_1000_id, Receiptor_Parameter{ 7500, false, 0, 0 }));
      post_operation::ext extension;
      extension.post_type = post_operation::Post_Type_Post;
      extension.forward_price = 10000 * prec;
      extension.receiptors = receiptors;
      extension.license_lid = 1;
      extension.permission_flags = 31;

      create_post({ u_1000_private_key, u_1001_private_key }, u_1001_id, u_1000_id, "6666666", "document name",
         "document body", "extra", optional<account_uid_type>(), optional<account_uid_type>(),
         optional<post_pid_type>(), extension);

      const uint64_t cycles = 200000;
      uint64_t total_time = 0;
      std::vector<signed_transaction> transactions;
      transactions.reserve(cycles);

      flat_map<account_uid_type, fc::ecc::private_key> account_map;
      actor(1003, cycles, account_map);

      //score test
      {
         score_create_operation score_op;
         score_op.platform = u_1001_id;
         score_op.poster = u_1000_id;
         score_op.post_pid = 1;
         score_op.score = 5;
         score_op.csaf = 20;

         for (const auto& a : account_map)
         {
            score_op.from_account_uid = a.first;
            signed_transaction tx;
            tx.operations.push_back(score_op);
            set_operation_fees(tx, db.current_fee_schedule());
            test::set_expiration(db, tx);
            tx.validate();
            transactions.push_back(tx);
            tx.operations.clear();
            add_csaf_for_account(a.first, 10000);
            account_manage(a.first, { true, true, true });
         }

         auto start = fc::time_point::now();
         for (uint32_t i = 0; i < cycles; ++i)
         {
            auto result = db.apply_transaction(transactions[i]);
         }
         auto end = fc::time_point::now();
         auto elapsed = end - start;
         total_time += elapsed.count();
         //result: Create 2712 score/s over 73743ms
         wlog("Create ${aps} score/s over ${total}ms",
            ("aps", (cycles * 1000000) / elapsed.count())("total", elapsed.count() / 1000));

      }

      transactions.clear();

      //reward
      {
         reward_operation reward_op;
         reward_op.platform = u_1001_id;
         reward_op.poster = u_1000_id;
         reward_op.post_pid = 1;
         reward_op.amount = asset(100000);
         for (const auto& a : account_map)
         {
            reward_op.from_account_uid = a.first;
            signed_transaction tx;
            tx.operations.push_back(reward_op);
            set_operation_fees(tx, db.current_fee_schedule());
            test::set_expiration(db, tx);
            tx.validate();
            transactions.push_back(tx);
            tx.operations.clear();
            transfer(committee_account, a.first, _core(100));
         }

         auto start = fc::time_point::now();
         for (uint32_t i = 0; i < cycles; ++i)
         {
            auto result = db.apply_transaction(transactions[i]);
         }
         auto end = fc::time_point::now();
         auto elapsed = end - start;
         total_time += elapsed.count();
         //result: 2639 reward/s over 75761ms
         wlog("${aps} reward/s over ${total}ms",
            ("aps", (cycles * 1000000) / elapsed.count())("total", elapsed.count() / 1000));

      }
   }
   catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}


BOOST_AUTO_TEST_CASE( transfer_benchmark )
{
   try{
      flat_map<account_uid_type, fc::ecc::private_key> account_map;
      actor(1003, 200000, account_map);

      const uint64_t cycles = 200000;
      std::vector<signed_transaction> transactions;
      transactions.reserve(cycles);

      transfer_operation trans_op;
      trans_op.from = committee_account;
      trans_op.amount = asset(1000);

      for (const auto& a : account_map)
      {
         trans_op.to = a.first;
         signed_transaction tx;
         tx.operations.push_back(trans_op);
         set_operation_fees(tx, db.current_fee_schedule());
         test::set_expiration(db, tx);
         tx.validate();
         transactions.push_back(tx);
         tx.operations.clear();
      }
      auto start = fc::time_point::now();
      for (uint32_t i = 0; i < cycles; ++i)
      {
         auto result = db.apply_transaction(transactions[i]);
      }
      auto end = fc::time_point::now();
      auto elapsed = end - start;
      //result: 3035 transfer/s over 65881ms
      wlog("${aps} transfer/s over ${total}ms",
         ("aps", (cycles * 1000000) / elapsed.count())("total", elapsed.count() / 1000));
   }
   catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}


// See https://bitshares.org/blog/2015/06/08/measuring-performance/
// (note this is not the original test mentioned in the above post, but was
//  recreated later according to the description)
BOOST_AUTO_TEST_CASE(one_hundred_k_benchmark)
{
   // try {
   //     ACTORS((alice));
   //     fund(alice, asset(10000000));
   //     db._undo_db.disable(); // Blog post mentions replay, this implies no undo

   //     const fc::ecc::private_key nathan_key = fc::ecc::private_key::generate();
   //     const fc::ecc::public_key  nathan_pub = nathan_key.get_public_key();;
   //     const auto& committee_account = account_id_type()(db);

   //     const uint64_t cycles = 200000;
   //     uint64_t total_time = 0;
   //     uint64_t total_count = 0;
   //     std::vector<account_id_type> accounts;
   //     accounts.reserve(cycles + 1);
   //     std::vector<asset_id_type> assets;
   //     assets.reserve(cycles);

   //     std::vector<signed_transaction> transactions;
   //     transactions.reserve(cycles);

   //     {
   //         account_create_operation aco;
   //         aco.name = "a1";
   //         aco.registrar = committee_account.id;
   //         aco.owner = authority(1, public_key_type(nathan_pub), 1);
   //         aco.active = authority(1, public_key_type(nathan_pub), 1);
   //         aco.options.memo_key = nathan_pub;
   //         aco.options.voting_account = GRAPHENE_PROXY_TO_SELF_ACCOUNT;
   //         aco.options.num_committee = 0;
   //         aco.options.num_witness = 0;
   //         aco.fee = db.current_fee_schedule().calculate_fee(aco);
   //         trx.clear();
   //         test::set_expiration(db, trx);
   //         for (uint32_t i = 0; i < cycles; ++i)
   //         {
   //             aco.name = "a" + fc::to_string(i);
   //             trx.operations.push_back(aco);
   //             transactions.push_back(trx);
   //             trx.operations.clear();
   //             ++total_count;
   //         }

   //         auto start = fc::time_point::now();
   //         for (uint32_t i = 0; i < cycles; ++i)
   //         {
   //             auto result = db.apply_transaction(transactions[i], ~0);
   //             accounts[i] = result.operation_results[0].get<object_id_type>();
   //         }
   //         auto end = fc::time_point::now();
   //         auto elapsed = end - start;
   //         total_time += elapsed.count();
   //         wlog("Create ${aps} accounts/s over ${total}ms",
   //             ("aps", (cycles * 1000000) / elapsed.count())("total", elapsed.count() / 1000));
   //     }

   //{
   //    accounts[cycles] = accounts[0];
   //    transfer_operation to1;
   //    to1.from = committee_account.id;
   //    to1.amount = asset(1000000);
   //    to1.fee = asset(10);
   //    transfer_operation to2;
   //    to2.amount = asset(100);
   //    to2.fee = asset(10);
   //    for (uint32_t i = 0; i < cycles; ++i)
   //    {
   //        to1.to = accounts[i];
   //        to2.from = accounts[i];
   //        to2.to = accounts[i + 1];
   //        trx.operations.push_back(to1);
   //        ++total_count;
   //        trx.operations.push_back(to2);
   //        ++total_count;
   //        transactions[i] = trx;
   //        trx.operations.clear();
   //    }

   //    auto start = fc::time_point::now();
   //    for (uint32_t i = 0; i < cycles; ++i)
   //        db.apply_transaction(transactions[i], ~0);
   //    auto end = fc::time_point::now();
   //    auto elapsed = end - start;
   //    total_time += elapsed.count();
   //    wlog("${aps} transfers/s over ${total}ms",
   //        ("aps", (2 * cycles * 1000000) / elapsed.count())("total", elapsed.count() / 1000));
   //    trx.clear();
   //}

   //{
   //    asset_create_operation aco;
   //    aco.fee = asset(100000);
   //    aco.common_options.core_exchange_rate = asset(1) / asset(1, asset_id_type(1));
   //    for (uint32_t i = 0; i < cycles; ++i)
   //    {
   //        aco.issuer = accounts[i];
   //        aco.symbol = "ASSET" + fc::to_string(i);
   //        trx.operations.push_back(aco);
   //        ++total_count;
   //        transactions[i] = trx;
   //        trx.operations.clear();
   //    }

   //    auto start = fc::time_point::now();
   //    for (uint32_t i = 0; i < cycles; ++i)
   //    {
   //        auto result = db.apply_transaction(transactions[i], ~0);
   //        assets[i] = result.operation_results[0].get<object_id_type>();
   //    }
   //    auto end = fc::time_point::now();
   //    auto elapsed = end - start;
   //    total_time += elapsed.count();
   //    wlog("${aps} asset create/s over ${total}ms",
   //        ("aps", (cycles * 1000000) / elapsed.count())("total", elapsed.count() / 1000));
   //    trx.clear();
   //}

   //{
   //    asset_issue_operation aio;
   //    aio.fee = asset(10);
   //    for (uint32_t i = 0; i < cycles; ++i)
   //    {
   //        aio.issuer = accounts[i];
   //        aio.issue_to_account = accounts[i + 1];
   //        aio.asset_to_issue = asset(10, assets[i]);
   //        trx.operations.push_back(aio);
   //        ++total_count;
   //        transactions[i] = trx;
   //        trx.operations.clear();
   //    }

   //    auto start = fc::time_point::now();
   //    for (uint32_t i = 0; i < cycles; ++i)
   //        db.apply_transaction(transactions[i], ~0);
   //    auto end = fc::time_point::now();
   //    auto elapsed = end - start;
   //    total_time += elapsed.count();
   //    wlog("${aps} issuances/s over ${total}ms",
   //        ("aps", (cycles * 1000000) / elapsed.count())("total", elapsed.count() / 1000));
   //    trx.clear();
   //}

   //wlog("${total} operations in ${total_time}ms => ${avg} ops/s on average",
   //    ("total", total_count)("total_time", total_time / 1000)
   //    ("avg", (total_count * 1000000) / total_time));

   //db._undo_db.enable();
   // } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

//#define BOOST_TEST_MODULE "C++ Unit Tests for Graphene Blockchain Database"
#include <cstdlib>
#include <iostream>
#include <boost/test/included/unit_test.hpp>

boost::unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]) {
   std::srand(time(NULL));
   std::cout << "Random number generator seeded to " << time(NULL) << std::endl;
   return nullptr;
}
