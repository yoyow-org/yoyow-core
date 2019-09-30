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
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE( performance_tests, database_fixture )

BOOST_AUTO_TEST_CASE( sigcheck_benchmark )
{
   //fc::ecc::private_key nathan_key = fc::ecc::private_key::generate();
   //auto digest = fc::sha256::hash("hello");
   //auto sig = nathan_key.sign_compact( digest );
   //auto start = fc::time_point::now();
   //for( uint32_t i = 0; i < 100000; ++i )
   //   auto pub = fc::ecc::public_key( sig, digest );
   //auto end = fc::time_point::now();
   //auto elapsed = end-start;
   //wdump( ((100000.0*1000000.0) / elapsed.count()) );
}

BOOST_AUTO_TEST_CASE(post_performance_test)
{
    try{
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

    const uint64_t cycles = 20000;
    uint64_t total_time = 0;
    std::vector<signed_transaction> transactions;
    transactions.reserve(cycles);

    post_operation::ext extension;
    extension.post_type = post_operation::Post_Type_Post;
    extension.forward_price = 10*prec;
    extension.license_lid = 1;
    extension.permission_flags = post_object::Post_Permission_Forward |
                                 post_object::Post_Permission_Liked |
                                 post_object::Post_Permission_Buyout |
                                 post_object::Post_Permission_Comment |
                                 post_object::Post_Permission_Reward;

    const _account_statistics_object& poster_account_statistics = db.get_account_statistics_by_uid(u_1000_id);
    post_operation create_op;
    create_op.platform = u_9000_id;
    create_op.poster = u_1000_id;
    create_op.hash_value = "6666666";
    create_op.extra_data = "extra";
    create_op.title = "document name";
    create_op.body = "document body";
    create_op.extensions = graphene::chain::extension<post_operation::ext>();
    create_op.extensions->value = extension;

    for (uint32_t i = 0; i < cycles; ++i){
        create_op.post_pid = poster_account_statistics.last_post_sequence + i + 1;
        signed_transaction tx;
        tx.operations.push_back(create_op);
        set_operation_fees(tx, db.current_fee_schedule());
        test::set_expiration(db, tx);
        tx.validate();
        transactions.push_back(tx);
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

    }
    catch (fc::exception& e) {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(comment_performance_test)
{
    try{
        //comment post test
        ACTORS((1000)(9000));
        const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
        auto _core = [&](int64_t x) -> asset
        {  return asset(x*prec);    };
        transfer(committee_account, u_9000_id, _core(10000));
        add_csaf_for_account(u_9000_id, 10000);

        // post
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

        post_operation::ext extension;
        extension.post_type = post_operation::Post_Type_Post;
        extension.forward_price = 10 * prec;
        extension.license_lid = 1;
        extension.permission_flags = post_object::Post_Permission_Forward |
            post_object::Post_Permission_Liked |
            post_object::Post_Permission_Buyout |
            post_object::Post_Permission_Comment |
            post_object::Post_Permission_Reward;

        create_post(sign_keys1, u_9000_id, u_1000_id, "6666666", "document name", "document body", "extra", optional<account_uid_type>(), optional<account_uid_type>(), optional<post_pid_type>(), extension);

        const uint64_t cycles = 20000;
        uint64_t total_time = 0;
        std::vector<signed_transaction> transactions;
        transactions.reserve(cycles);
        const fc::ecc::private_key nathan_key = fc::ecc::private_key::generate();
        const fc::ecc::public_key  nathan_pub = nathan_key.get_public_key();
        std::vector<account_uid_type> accounts;
        accounts.reserve(cycles + 1);

        for (uint32_t i = 0; i < cycles; ++i){
            account_uid_type uid = graphene::chain::calc_account_uid(10101 + i);
            accounts.push_back(create_account(uid, "a" + fc::to_string(i), nathan_pub).uid);
            account_manage(accounts[i], account_manage_operation::opt{ true, true, true });
            add_csaf_for_account(accounts[i], 1000);
            //transfer(committee_account, accounts[i], _core(200));
            //transfer_extension({ nathan_key }, accounts[i], accounts[i], _core(100), "", true, false);
            account_auth_platform({ nathan_key }, accounts[i], u_9000_id, 1000 * prec, account_auth_platform_object::Platform_Permission_Forward |
                account_auth_platform_object::Platform_Permission_Liked |
                account_auth_platform_object::Platform_Permission_Buyout |
                account_auth_platform_object::Platform_Permission_Comment |
                account_auth_platform_object::Platform_Permission_Reward |
                account_auth_platform_object::Platform_Permission_Post |
                account_auth_platform_object::Platform_Permission_Content_Update);
        }

        extension.post_type = post_operation::Post_Type_Comment;
        for (uint32_t i = 0; i < cycles; ++i){
            const _account_statistics_object& comment_account_statistics = db.get_account_statistics_by_uid(accounts[i]);
            post_operation comment_op;
            comment_op.platform = u_9000_id;
            comment_op.poster = accounts[i];
            comment_op.post_pid = comment_account_statistics.last_post_sequence + 1;
            comment_op.hash_value = "6666666";
            comment_op.extra_data = "extra";
            comment_op.title = "comment name";
            comment_op.body = "comment body";
            comment_op.origin_platform = u_9000_id;
            comment_op.origin_poster = u_1000_id;
            comment_op.origin_post_pid = 1;
            comment_op.extensions = graphene::chain::extension<post_operation::ext>();
            comment_op.extensions->value = extension;

            signed_transaction tx;
            tx.operations.push_back(comment_op);
            set_operation_fees(tx, db.current_fee_schedule());
            test::set_expiration(db, tx);
            tx.validate();
            transactions.push_back(tx);
        }

        auto start2 = fc::time_point::now();
        for (uint32_t i = 0; i < cycles; ++i)
        {
            auto result = db.apply_transaction(transactions[i]);
        }
        auto end2 = fc::time_point::now();
        auto elapsed2 = end2 - start2;
        total_time += elapsed2.count();
        wlog("Comment ${aps} post/s over ${total}ms",
            ("aps", (cycles * 1000000) / elapsed2.count())("total", elapsed2.count() / 1000));
    }
    catch (fc::exception& e) {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(forward_performance_test)
{
    try{
        ACTORS((1000)(9000));
        const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
        auto _core = [&](int64_t x) -> asset
        {  return asset(x*prec);    };
        transfer(committee_account, u_9000_id, _core(10000));
        add_csaf_for_account(u_9000_id, 10000);

        // post
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

        post_operation::ext extension;
        extension.post_type = post_operation::Post_Type_Post;
        extension.forward_price = 10 * prec;
        extension.license_lid = 1;
        extension.permission_flags = post_object::Post_Permission_Forward |
            post_object::Post_Permission_Liked |
            post_object::Post_Permission_Buyout |
            post_object::Post_Permission_Comment |
            post_object::Post_Permission_Reward;

        create_post(sign_keys1, u_9000_id, u_1000_id, "6666666", "document name", "document body", "extra", optional<account_uid_type>(), optional<account_uid_type>(), optional<post_pid_type>(), extension);

        const uint64_t cycles = 20000;
        uint64_t total_time = 0;
        std::vector<signed_transaction> transactions;
        transactions.reserve(cycles);
        const fc::ecc::private_key nathan_key = fc::ecc::private_key::generate();
        const fc::ecc::public_key  nathan_pub = nathan_key.get_public_key();
        std::vector<account_uid_type> accounts;
        accounts.reserve(cycles + 1);
        for (uint32_t i = 0; i < cycles; ++i){
            account_uid_type uid = graphene::chain::calc_account_uid(10101 + i);
            accounts.push_back(create_account(uid, "a" + fc::to_string(i), nathan_pub).uid);
            //account_manage(accounts[i], account_manage_operation::opt{ true, true, true });
            add_csaf_for_account(accounts[i], 1000);
            transfer(committee_account, accounts[i], _core(200));
            transfer_extension({ nathan_key }, accounts[i], accounts[i], _core(100), "", true, false);
            account_auth_platform({ nathan_key }, accounts[i], u_9000_id, 1000 * prec, account_auth_platform_object::Platform_Permission_Forward |
                account_auth_platform_object::Platform_Permission_Liked |
                account_auth_platform_object::Platform_Permission_Buyout |
                account_auth_platform_object::Platform_Permission_Comment |
                account_auth_platform_object::Platform_Permission_Reward |
                account_auth_platform_object::Platform_Permission_Post |
                account_auth_platform_object::Platform_Permission_Content_Update);
        }

        //forward post test
        extension.post_type = post_operation::Post_Type_forward;
        for (uint32_t i = 0; i < cycles; ++i){
            const _account_statistics_object& forward_account_statistics = db.get_account_statistics_by_uid(accounts[i]);
            post_operation forward_op;
            forward_op.platform = u_9000_id;
            forward_op.poster = accounts[i];
            forward_op.post_pid = forward_account_statistics.last_post_sequence + 1;
            forward_op.hash_value = "6666666";
            forward_op.extra_data = "extra";
            forward_op.title = "forward_op name";
            forward_op.body = "forward_op body";
            forward_op.origin_platform = u_9000_id;
            forward_op.origin_poster = u_1000_id;
            forward_op.origin_post_pid = 1;
            forward_op.extensions = graphene::chain::extension<post_operation::ext>();
            forward_op.extensions->value = extension;

            signed_transaction tx;
            tx.operations.push_back(forward_op);
            set_operation_fees(tx, db.current_fee_schedule());
            test::set_expiration(db, tx);
            tx.validate();
            transactions.push_back(tx);
        }

        auto start3 = fc::time_point::now();
        for (uint32_t i = 0; i < cycles; ++i)
        {
            auto result = db.apply_transaction(transactions[i]);
        }
        auto end3 = fc::time_point::now();
        auto elapsed3 = end3 - start3;
        total_time += elapsed3.count();
        wlog("Forward ${aps} post/s over ${total}ms",
            ("aps", (cycles * 1000000) / elapsed3.count())("total", elapsed3.count() / 1000));
    }
    catch (fc::exception& e) {
        edump((e.to_detail_string()));
        throw;
    }
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


BOOST_AUTO_TEST_CASE(content_award_performance_test_1)
{
   try{

      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);

      // Return number of core shares (times precision)
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };

      const uint64_t score_count = 10000;
      flat_map<account_uid_type, fc::ecc::private_key> score_map;
      int cycle = 20;
      for (int i = 0; i < cycle; ++i)
      {
         actor(1003+i*score_count, score_count, score_map);
         generate_blocks(4);
      }

      //actor(500000, cycles, score_map);

      committee_update_global_content_parameter_item_type item;
      item.value = { 15000, 15000, 1000, 31536000, 10, 10000000000000, 10000000000000, 10000000000000, 1000, 100 };
      int current_block_num = db.head_block_num();
      committee_proposal_create(genesis_state.initial_accounts.at(0).uid, { item }, current_block_num + 5, voting_opinion_type::opinion_for, current_block_num + 5, current_block_num+5);
      for (int i = 1; i < 5; ++i)
      committee_proposal_vote(genesis_state.initial_accounts.at(i).uid, 1, voting_opinion_type::opinion_for);
      generate_blocks(5);

      //1 platform, 1 post/per platform, 20 0000 scores/per post
      ACTORS((300000)(400001));

      transfer(committee_account, u_300000_id, _core(100000));
      create_platform(u_300000_id, "platform", _core(10000), "www.123456789.com", "", { u_300000_private_key });
      create_license(u_300000_id, 6, "999999999", "license title", "license body", "extra", { u_300000_private_key });

      post_operation::ext extensions;
      extensions.license_lid = 1;
      create_post({ u_400001_private_key, u_300000_private_key }, u_300000_id, u_400001_id, "", "", "", "",
         optional<account_uid_type>(),
         optional<account_uid_type>(),
         optional<post_pid_type>(),
         extensions);

      int count = 0;
      int block_num = 4000;
      uint64_t gap = score_count  / 1000;
      for (auto a : score_map)
      {
         ++count;
         if (count == gap && block_num > 0)
         {
            generate_blocks(4);
            count=0;
            block_num -= 4;
         }
         score_a_post({ a.second }, a.first, u_300000_id, u_400001_id, 1, 5, 10);
      }

      generate_block(~0,init_account_priv_key,block_num + 999);
      auto start = fc::time_point::now();
      wlog("1 platform, 1 post/per platform, 20 0000 scores/per post, content award begin>>>>>>>>>>${num}", ("num", db.head_block_num()));
      generate_block();
      auto end = fc::time_point::now();
      auto elapsed = end - start;
      wlog("1 platform, 1 post/per platform, 20 0000 scores/per post, content award spend ${total}ms",
         ("total", elapsed.count() / 1000));
   }
   catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(content_award_performance_test_2)
{
   try{

      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);

      // Return number of core shares (times precision)
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };

      const uint64_t score_count = 10000;

      flat_map<account_uid_type, fc::ecc::private_key> score_map;
      actor(1003, score_count, score_map);

      generate_blocks(4);
      actor(31003, score_count, score_map);

      int current_block_num = db.head_block_num();

      committee_update_global_content_parameter_item_type item;
      item.value = { 15000, 15000, 1000, 31536000, 10, 10000000000000, 10000000000000, 10000000000000, 1000, 100 };
      committee_proposal_create(genesis_state.initial_accounts.at(0).uid, { item }, current_block_num + 5, voting_opinion_type::opinion_for, current_block_num + 5, current_block_num+5);
      for (int i = 1; i < 5; ++i)
         committee_proposal_vote(genesis_state.initial_accounts.at(i).uid, 1, voting_opinion_type::opinion_for);
      generate_blocks(5);

      const uint64_t post_count = 200;
      flat_map<account_uid_type, fc::ecc::private_key> post_map;
      actor(400002, post_count, post_map);

      ACTORS((300001));

      transfer(committee_account, u_300001_id, _core(20000));
      create_platform(u_300001_id, "platform", _core(10000), "www.123456789.com", "", { u_300001_private_key });
      create_license(u_300001_id, 6, "999999999", "license title", "license body", "extra", { u_300001_private_key });

      post_operation::ext extensions;
      extensions.license_lid = 1;

      uint64_t gap = score_count * post_count / 1000;

      int count = 0;
      int block_num = 4000;
      for (const auto& p : post_map)
      {
         create_post({ p.second, u_300001_private_key }, u_300001_id, p.first, "", "", "", "",
            optional<account_uid_type>(),
            optional<account_uid_type>(),
            optional<post_pid_type>(),
            extensions);

         for (auto a : score_map)
         {
            ++count;
            if (count == gap && block_num > 0)
            {
               generate_blocks(4);
               count = 0;
               block_num -= 4;
            }
            score_a_post({ a.second }, a.first, u_300001_id, p.first, 1, 5, 10);
         }
      }

      generate_block(~0,init_account_priv_key,block_num + 999);
      wlog("1 platform, 20 0000 post/per platform, 20 0000 score/per post, content award test begin------${num}", ("num", db.head_block_num()));
      auto start = fc::time_point::now();
      generate_block();
      auto end = fc::time_point::now();
      auto elapsed = end - start;
      wlog("1 platform, 20 0000 post/per platform, 20 0000 score/per post, content award spend ${total}ms",
         ("total", elapsed.count() / 1000));
   }
   catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(content_award_performance_test_3)
{
   try{

      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);

      // Return number of core shares (times precision)
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };

      const uint64_t score_count = 5000;

      flat_map<account_uid_type, fc::ecc::private_key> score_map;
      actor(1003, score_count, score_map);

      committee_update_global_content_parameter_item_type item;
      item.value = { 15000, 15000, 1000, 31536000, 10, 10000000000000, 10000000000000, 10000000000000, 1000, 100 };
      committee_proposal_create(genesis_state.initial_accounts.at(0).uid, { item }, 5, voting_opinion_type::opinion_for, 5, 5);
      for (int i = 1; i < 5; ++i)
         committee_proposal_vote(genesis_state.initial_accounts.at(i).uid, 1, voting_opinion_type::opinion_for);
      generate_blocks(4);

      const uint64_t post_count = 20;
      flat_map<account_uid_type, fc::ecc::private_key> post_map;
      actor(400002, post_count, post_map);

      //10000 platforms, 20 0000 posts/per platform, 20 0000 scores/per post
      const uint64_t platform_count = 20;

      flat_map<account_uid_type, fc::ecc::private_key> platform_map;
      actor(700000, platform_count, platform_map);

      uint64_t gap = score_count * post_count * platform_count / 1000;

      int i = 0;
      int count = 0;
      int block_num = 4000;
      for (const auto& p : platform_map)
      {
         i++;
         transfer(committee_account, p.first, _core(100000));
         create_platform(p.first, "platform", _core(10000), "www.123456789.com", "", { p.second });
         create_license(p.first, 6, "999999999", "license title", "license body", "extra", { p.second });

         post_operation::ext extensions;
         extensions.license_lid = 1;

         for (const auto& a : post_map)
         {
            create_post({ a.second, p.second }, p.first, a.first, "", "", "", "",
               optional<account_uid_type>(),
               optional<account_uid_type>(),
               optional<post_pid_type>(),
               extensions);
         }
      }
      
      generate_blocks(99);

      const auto& post_idx = db.get_index_type<post_index>().indices().get<by_id>();
      auto itr = post_idx.begin();
      while (itr != post_idx.end())
      {
         for (auto s : score_map)
         {
            ++count;
            if (count == gap && block_num > 0)
            {
               generate_blocks(4);
               count = 0;
               block_num -= 4;
            }
            score_a_post({ s.second }, s.first, itr->platform, itr->poster,itr->post_pid, 5, 10);
         }
         itr++;
      }

      generate_block(~0,init_account_priv_key,block_num + 900);

      wlog("10000 platforms, 20 0000 posts/per platform, 20 0000 scores/per post, content award begin........,${num}",("num", db.head_block_num()));
      auto start = fc::time_point::now();
      generate_block();
      auto end = fc::time_point::now();
      auto elapsed = end - start;
      wlog("10000 platforms, 20 0000 posts/per platform, 20 0000 scores/per post, content award spend ${total}ms",
         ("total", elapsed.count() / 1000));

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
   const char* genesis_timestamp_str = getenv("GRAPHENE_TESTING_GENESIS_TIMESTAMP");
   if (genesis_timestamp_str != nullptr)
   {
       GRAPHENE_TESTING_GENESIS_TIMESTAMP = std::stoul(genesis_timestamp_str);
   }
   std::cout << "GRAPHENE_TESTING_GENESIS_TIMESTAMP is " << GRAPHENE_TESTING_GENESIS_TIMESTAMP << std::endl;
   return nullptr;
}
