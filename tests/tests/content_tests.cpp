
#include <boost/test/unit_test.hpp>

#include <graphene/chain/database.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>

#include <graphene/chain/protocol/content.hpp>
#include <graphene/chain/content_object.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/witness_object.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/log/logger.hpp>
#include <boost/multiprecision/cpp_int.hpp>

#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE( operation_tests, database_fixture )

BOOST_AUTO_TEST_CASE(collect_csaf_test)
{
    try
    {
        ACTORS((1000)(2000));
        const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);

        collect_csaf({ u_1000_private_key }, u_1000_id, u_1000_id, 1000);

        const account_statistics_object& ants_1000 = db.get_account_statistics_by_uid(u_1000_id);
        BOOST_CHECK(ants_1000.csaf == 1000 * prec);

        //###############################################################
        collect_csaf_from_committee(u_2000_id, 1000);
        const account_statistics_object& ants_2000 = db.get_account_statistics_by_uid(u_2000_id);
        BOOST_CHECK(ants_2000.csaf == 1000 * prec);
    }
    catch (const fc::exception& e)
    {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(committee_proposal_test)
{
   try
   {
      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);

      // Return number of core shares (times precision)
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };

      // make sure the database requires our fee to be nonzero
      enable_fees();

      for (int i = 0; i < 5; ++i)
         add_csaf_for_account(genesis_state.initial_accounts.at(i).uid, 1000);

      generate_blocks(10);

      committee_update_global_content_parameter_item_type item;
      item.value = { 300, 300, 1000, 31536000, 10, 10000000000000, 10000000000000, 10000000000000, 1, 100 };
      committee_proposal_create(genesis_state.initial_accounts.at(0).uid, { item }, 100, voting_opinion_type::opinion_for, 100, 100);
      for (int i = 1; i < 5; ++i)
         committee_proposal_vote(genesis_state.initial_accounts.at(i).uid, 1, voting_opinion_type::opinion_for);

      generate_blocks(101);
      auto gap = db.get_global_properties().parameters.get_award_params();
     
      BOOST_REQUIRE_EQUAL(gap.content_award_interval,    300);
      BOOST_REQUIRE_EQUAL(gap.platform_award_interval,   300);
      BOOST_REQUIRE_EQUAL(gap.max_csaf_per_approval,     1000);
      BOOST_REQUIRE_EQUAL(gap.approval_expiration,       31536000);
      BOOST_REQUIRE_EQUAL(gap.min_effective_csaf.value,  10);
      BOOST_REQUIRE_EQUAL(gap.total_content_award_amount.value,            10000000000000);
      BOOST_REQUIRE_EQUAL(gap.total_platform_content_award_amount.value,   10000000000000);
      BOOST_REQUIRE_EQUAL(gap.total_platform_voted_award_amount.value,     10000000000000);
      BOOST_REQUIRE_EQUAL(gap.platform_award_min_votes, 1);
      BOOST_REQUIRE_EQUAL(gap.platform_award_requested_rank, 100);
   }
   catch (const fc::exception& e)
   {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(update_post_test)
{
   try{
      ACTORS((1000)(1001)
         (9000));

      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);

      // Return number of core shares (times precision)
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };

      transfer(committee_account, u_1000_id, _core(100000));
      transfer(committee_account, u_9000_id, _core(100000));

      add_csaf_for_account(u_1000_id, 10000);
      add_csaf_for_account(u_9000_id, 10000);

      create_platform(u_9000_id, "platform", _core(10000), "www.123456789.com", "", { u_9000_private_key });
      account_auth_platform({ u_1000_private_key }, u_1000_id, u_9000_id, 1000 * prec, 0x1F);
      create_license(u_9000_id, 6, "999999999", "license title", "license body", "extra", {u_9000_private_key});

      create_post({ u_1001_private_key, u_9000_private_key }, u_9000_id, u_1001_id, "", "", "", "",
         optional<account_uid_type>(),
         optional<account_uid_type>(),
         optional<post_pid_type>());

      post_update_operation::ext ext;
      ext.forward_price    = 100 * prec;
      ext.receiptor        = u_1001_id;
      ext.to_buyout        = true;
      ext.buyout_ratio     = 3000;
      ext.buyout_price     = 10000 * prec;    
      ext.license_lid      = 1;
      ext.permission_flags = 0xF;
      update_post({ u_1001_private_key, u_9000_private_key }, u_9000_id, u_1001_id, 1, "", "", "", "", ext);

      auto post_obj = db.get_post_by_platform(u_9000_id, u_1001_id, 1);
      Recerptor_Parameter parameter = post_obj.receiptors[u_1001_id];

      BOOST_CHECK(*(post_obj.forward_price) == 100 * prec);
      BOOST_CHECK(parameter.to_buyout == true);
      BOOST_CHECK(parameter.buyout_ratio == 3000);
      BOOST_CHECK(parameter.buyout_price == 10000 * prec);
      BOOST_CHECK(post_obj.license_lid == 1);
      BOOST_CHECK(post_obj.permission_flags == 0xF);
   }
   catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(score_test)
{
   try{
      ACTORS((1001)
         (1003)(1004)(1005)(1006)(1007)(1008)(1009)(1010)(1011)(1012)
         (9000));

      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);

      // Return number of core shares (times precision)
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };

      //transfer(committee_account, u_1000_id, _core(100000));
      transfer(committee_account, u_9000_id, _core(100000));

      flat_map<account_uid_type, fc::ecc::private_key> map = {
         { u_1003_id, u_1003_private_key }, { u_1004_id, u_1004_private_key }, { u_1005_id, u_1005_private_key },
         { u_1006_id, u_1006_private_key }, { u_1007_id, u_1007_private_key }, { u_1008_id, u_1008_private_key },
         { u_1009_id, u_1009_private_key }, { u_1010_id, u_1010_private_key }, { u_1011_id, u_1011_private_key },
         { u_1012_id, u_1012_private_key } };

      for (auto a : map)
         add_csaf_for_account(a.first, 10000);
      add_csaf_for_account(u_9000_id, 10000);

      create_platform(u_9000_id, "platform", _core(10000), "www.123456789.com", "", { u_9000_private_key });

      create_post({ u_1001_private_key, u_9000_private_key }, u_9000_id, u_1001_id, "", "", "", "",
         optional<account_uid_type>(),
         optional<account_uid_type>(),
         optional<post_pid_type>());

      for (auto a : map)
      {
         account_auth_platform({a.second}, a.first, u_9000_id, 1000 * prec, 0x1F);
         account_manage(a.first, {true, true, true});
         score_a_post({a.second, u_9000_private_key}, a.first, u_9000_id, u_1001_id, 1, 5, 10);
      }
       
      const auto& apt_idx = db.get_index_type<active_post_index>().indices().get<by_post_pid>();
      auto apt_itr = apt_idx.find(std::make_tuple(u_9000_id, u_1001_id, 1));     
      BOOST_CHECK(apt_itr != apt_idx.end());
      auto active_post = *apt_itr;
      BOOST_CHECK(active_post.total_amount == 10 * 10);
    
      for (auto a : map)
      {
         auto score_obj = db.get_score(u_9000_id, u_1001_id, 1, a.first);
         BOOST_CHECK(score_obj.score == 5);
         BOOST_CHECK(score_obj.csaf == 10);
         bool found = false;
         for (auto score : active_post.scores)
            if (score == score_obj.id)
            {
               found = true;
               break;
            }
         BOOST_CHECK(found == true);
      }        
   }
   catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

typedef boost::multiprecision::uint128_t uint128_t;
BOOST_AUTO_TEST_CASE(reward_test)
{
   try{
      ACTORS((1001)
         (1003)(1004)(1005)(1006)(1007)(1008)(1009)(1010)(1011)(1012)
         (9000));

      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);

      // Return number of core shares (times precision)
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };

      transfer(committee_account, u_9000_id, _core(100000));

      flat_map<account_uid_type, fc::ecc::private_key> map = {
         { u_1003_id, u_1003_private_key }, { u_1004_id, u_1004_private_key }, { u_1005_id, u_1005_private_key },
         { u_1006_id, u_1006_private_key }, { u_1007_id, u_1007_private_key }, { u_1008_id, u_1008_private_key },
         { u_1009_id, u_1009_private_key }, { u_1010_id, u_1010_private_key }, { u_1011_id, u_1011_private_key },
         { u_1012_id, u_1012_private_key } };

      for (auto a : map)
         add_csaf_for_account(a.first, 10000);
      add_csaf_for_account(u_9000_id, 10000);

      create_platform(u_9000_id, "platform", _core(10000), "www.123456789.com", "", { u_9000_private_key });

      create_post({ u_1001_private_key, u_9000_private_key }, u_9000_id, u_1001_id, "", "", "", "",
         optional<account_uid_type>(),
         optional<account_uid_type>(),
         optional<post_pid_type>());

      for (auto a : map)
      {
         transfer(committee_account, a.first, _core(100000));
         reward_post(a.first, u_9000_id, u_1001_id, 1, _core(1000), { a.second });
      }

      const auto& apt_idx = db.get_index_type<active_post_index>().indices().get<by_post_pid>();
      auto apt_itr = apt_idx.find(std::make_tuple(u_9000_id, u_1001_id, 1));
      BOOST_CHECK(apt_itr != apt_idx.end());
      auto active_post = *apt_itr;
      BOOST_CHECK(active_post.total_rewards.find(GRAPHENE_CORE_ASSET_AID) != active_post.total_rewards.end());
      BOOST_CHECK(active_post.total_rewards[GRAPHENE_CORE_ASSET_AID] == 10 * 1000 * prec);

      post_object post_obj = db.get_post_by_platform(u_9000_id, u_1001_id, 1);
      int64_t poster_earned = (post_obj.receiptors[u_1001_id].cur_ratio * uint128_t(100000000) / 10000).convert_to<int64_t>();
      int64_t platform_earned = 100000000 - poster_earned;

      auto act_1001 = db.get_account_statistics_by_uid(u_1001_id);
      BOOST_CHECK(act_1001.core_balance == poster_earned * 10);
      auto act_9000 = db.get_account_statistics_by_uid(u_9000_id);
      BOOST_CHECK(act_9000.core_balance == (platform_earned * 10 + 100000 * prec));

      for (auto a : map)
      {
         auto act = db.get_account_statistics_by_uid(a.first);
         BOOST_CHECK(act.core_balance == (100000 - 1000) * prec);
      }

   }
   catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}


BOOST_AUTO_TEST_CASE(post_platform_reward_test)
{
   try{
      ACTORS((1001)(9000));

      flat_map<account_uid_type, fc::ecc::private_key> score_map;
      actor(1003, 10, score_map);

      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);

      // Return number of core shares (times precision)
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };


      for (int i = 0; i < 5; ++i)
         add_csaf_for_account(genesis_state.initial_accounts.at(i).uid, 1000);
      transfer(committee_account, u_9000_id, _core(100000));
      generate_blocks(10);

      BOOST_TEST_MESSAGE("Turn on the reward mechanism, open content award and platform voted award");
      committee_update_global_content_parameter_item_type item;
      item.value = { 300, 300, 1000, 31536000, 10, 10000000000000, 10000000000000, 10000000000000, 1, 100 };
      committee_proposal_create(genesis_state.initial_accounts.at(0).uid, { item }, 100, voting_opinion_type::opinion_for, 100, 100);
      for (int i = 1; i < 5; ++i)
         committee_proposal_vote(genesis_state.initial_accounts.at(i).uid, 1, voting_opinion_type::opinion_for);
      generate_blocks(89);

      add_csaf_for_account(u_9000_id, 10000);
      create_platform(u_9000_id, "platform", _core(10000), "www.123456789.com", "", { u_9000_private_key });
      create_post({ u_1001_private_key, u_9000_private_key }, u_9000_id, u_1001_id, "", "", "", "",
         optional<account_uid_type>(),
         optional<account_uid_type>(),
         optional<post_pid_type>());

      account_manage_operation::opt options;
      options.can_rate = true;
      for (auto a : score_map)
      {
         add_csaf_for_account(a.first, 10000);
         account_auth_platform({ a.second }, a.first, u_9000_id, 1000 * prec, 0x1F);

         account_manage(GRAPHENE_NULL_ACCOUNT_UID, a.first, options);
         auto b = db.get_account_by_uid(a.first);
         score_a_post({ a.second, u_9000_private_key }, a.first, u_9000_id, u_1001_id, 1, 5, 10);
      }

      generate_blocks(100);

      uint128_t award_average = (uint128_t)10000000000000 * 300 / (86400 * 365);
      uint128_t post_earned = award_average * 10 * 10 / (10 * 10);

      uint64_t  poster_earned = (post_earned * 3000 / 10000).convert_to<uint64_t>();
      auto poster_act = db.get_account_statistics_by_uid(u_1001_id);
      BOOST_CHECK(poster_act.core_balance == poster_earned);
   }
   catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(transfer_extension_test)
{
    try{
        ACTORS((1000)(1001)(2000)(9000));

        const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);

        // Return number of core shares (times precision)
        auto _core = [&](int64_t x) -> asset
        {  return asset(x*prec);    };

        transfer(committee_account, u_1000_id, _core(10000));
        transfer(committee_account, u_1001_id, _core(10000));
        transfer(committee_account, u_2000_id, _core(10000));
        add_csaf_for_account(u_1000_id, 10000);
        add_csaf_for_account(u_1001_id, 10000);
        add_csaf_for_account(u_2000_id, 10000);
        add_csaf_for_account(u_9000_id, 10000);
        const account_statistics_object& temp = db.get_account_statistics_by_uid(u_1000_id);

        // make sure the database requires our fee to be nonzero
        enable_fees();

        flat_set<fc::ecc::private_key> sign_keys;
        sign_keys.insert(u_1000_private_key);
        transfer_extension(sign_keys, u_1000_id, u_1000_id, _core(6000), "", true, false);
        const account_statistics_object& ant1000 = db.get_account_statistics_by_uid(u_1000_id);
        BOOST_CHECK(ant1000.prepaid == 6000 * prec);
        BOOST_CHECK(ant1000.core_balance == 4000 * prec);

        transfer_extension(sign_keys, u_1000_id, u_1001_id, _core(5000), "", false, true);
        const account_statistics_object& ant1000_1 = db.get_account_statistics_by_uid(u_1000_id);
        const account_statistics_object& ant1001 = db.get_account_statistics_by_uid(u_1001_id);
        BOOST_CHECK(ant1000_1.prepaid == 1000 * prec);
        BOOST_CHECK(ant1001.core_balance == 15000 * prec);

        flat_set<fc::ecc::private_key> sign_keys1;
        sign_keys1.insert(u_1001_private_key);
        transfer_extension(sign_keys1, u_1001_id, u_1000_id, _core(15000), "", true, true);
        const account_statistics_object& ant1000_2 = db.get_account_statistics_by_uid(u_1000_id);
        BOOST_CHECK(ant1000_2.prepaid == 1000 * prec);
        BOOST_CHECK(ant1000_2.core_balance == 19000 * prec);

        transfer_extension(sign_keys, u_1000_id, u_1001_id, _core(1000), "", false, false);
        const account_statistics_object& ant1001_2 = db.get_account_statistics_by_uid(u_1001_id);
        const account_statistics_object& ant1000_3 = db.get_account_statistics_by_uid(u_1000_id);
        BOOST_CHECK(ant1001_2.prepaid == 1000 * prec);
        BOOST_CHECK(ant1000_3.prepaid == 0);

        account_auth_platform({ u_2000_private_key }, u_2000_id, u_9000_id, 1000 * prec, account_statistics_object::Platform_Permission_Forward |
                                                                                         account_statistics_object::Platform_Permission_Liked |
                                                                                         account_statistics_object::Platform_Permission_Buyout |
                                                                                         account_statistics_object::Platform_Permission_Comment |
                                                                                         account_statistics_object::Platform_Permission_Reward |
                                                                                         account_statistics_object::Platform_Permission_Transfer);
        transfer_extension({u_2000_private_key}, u_2000_id, u_2000_id, _core(10000), "", true, false);
        transfer_extension({u_9000_private_key}, u_2000_id, u_9000_id, _core(1000), "", false, true);
        const account_statistics_object& ant2000 = db.get_account_statistics_by_uid(u_2000_id);
        const account_statistics_object& ant9000 = db.get_account_statistics_by_uid(u_9000_id);
        BOOST_CHECK(ant2000.prepaid == 9000 * prec);
        BOOST_CHECK(ant9000.core_balance == 1000 * prec);

    } catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(account_auth_platform_test)
{
    try{
        ACTORS((1000)(9000));
        const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
        auto _core = [&](int64_t x) -> asset
        {  return asset(x*prec);    };
        transfer(committee_account, u_1000_id, _core(10000));
        transfer(committee_account, u_9000_id, _core(10000));
        add_csaf_for_account(u_1000_id, 10000);
        add_csaf_for_account(u_9000_id, 10000);

        flat_set<fc::ecc::private_key> sign_keys;
        sign_keys.insert(u_9000_private_key);
        create_platform(u_9000_id, "platform", _core(10000), "www.123456789.com", "", sign_keys);

        flat_set<fc::ecc::private_key> sign_keys1;
        sign_keys1.insert(u_1000_private_key);
        account_auth_platform(sign_keys1, u_1000_id, u_9000_id, 1000 * prec, account_statistics_object::Platform_Permission_Forward |
                                                                             account_statistics_object::Platform_Permission_Liked |
                                                                             account_statistics_object::Platform_Permission_Buyout |
                                                                             account_statistics_object::Platform_Permission_Comment |
                                                                             account_statistics_object::Platform_Permission_Reward |
                                                                             account_statistics_object::Platform_Permission_Transfer);

        const account_statistics_object& ant1000 = db.get_account_statistics_by_uid(u_1000_id);
        auto iter = ant1000.prepaids_for_platform.find(u_9000_id);
        BOOST_CHECK(iter != ant1000.prepaids_for_platform.end());
        BOOST_CHECK(iter->second.max_limit == 1000 * prec);
        BOOST_CHECK(iter->second.permission_flags & account_statistics_object::Platform_Permission_Forward);
        BOOST_CHECK(iter->second.permission_flags & account_statistics_object::Platform_Permission_Liked);
        BOOST_CHECK(iter->second.permission_flags & account_statistics_object::Platform_Permission_Buyout);
        BOOST_CHECK(iter->second.permission_flags & account_statistics_object::Platform_Permission_Comment);
        BOOST_CHECK(iter->second.permission_flags & account_statistics_object::Platform_Permission_Reward);
        BOOST_CHECK(iter->second.permission_flags & account_statistics_object::Platform_Permission_Transfer);


        account_auth_platform(sign_keys1, u_1000_id, u_9000_id, 6000 * prec, 0);

        const account_statistics_object& ant10001 = db.get_account_statistics_by_uid(u_1000_id);
        auto iter2 = ant10001.prepaids_for_platform.find(u_9000_id);
        BOOST_CHECK(iter2 != ant10001.prepaids_for_platform.end());
        BOOST_CHECK(iter2->second.max_limit == 6000 * prec);
        BOOST_CHECK((iter2->second.permission_flags & account_statistics_object::Platform_Permission_Forward)==0);
        BOOST_CHECK((iter2->second.permission_flags & account_statistics_object::Platform_Permission_Liked)==0);
        BOOST_CHECK((iter2->second.permission_flags & account_statistics_object::Platform_Permission_Buyout)==0);
        BOOST_CHECK((iter2->second.permission_flags & account_statistics_object::Platform_Permission_Comment)==0);
        BOOST_CHECK((iter2->second.permission_flags & account_statistics_object::Platform_Permission_Reward)==0);
        BOOST_CHECK((iter2->second.permission_flags & account_statistics_object::Platform_Permission_Transfer) == 0);
    }
    catch (fc::exception& e) {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(license_test)
{
    try{
        ACTORS((1000)(9000));
        const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
        auto _core = [&](int64_t x) -> asset
        {  return asset(x*prec);    };
        transfer(committee_account, u_9000_id, _core(10000));
        add_csaf_for_account(u_9000_id, 10000);

        flat_set<fc::ecc::private_key> sign_keys;
        sign_keys.insert(u_9000_private_key);
        create_platform(u_9000_id, "platform", _core(10000), "www.123456789.com", "", sign_keys);

        create_license(u_9000_id, 6, "999999999", "license title", "license body", "extra", sign_keys);

        const license_object& license = db.get_license_by_platform(u_9000_id, 1);
        BOOST_CHECK(license.license_type == 6);
        BOOST_CHECK(license.hash_value   == "999999999");
        BOOST_CHECK(license.extra_data   == "extra");
        BOOST_CHECK(license.title        == "license title");
        BOOST_CHECK(license.body         == "license body");
    }
    catch (fc::exception& e) {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(post_test)
{
    try{
        ACTORS((1000)(2000)(9000));
        const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
        auto _core = [&](int64_t x) -> asset
        {  return asset(x*prec);    };
        transfer(committee_account, u_1000_id, _core(10000));
        transfer(committee_account, u_2000_id, _core(10000));
        transfer(committee_account, u_9000_id, _core(10000));
        add_csaf_for_account(u_1000_id, 10000);
        add_csaf_for_account(u_2000_id, 10000);
        add_csaf_for_account(u_9000_id, 10000);

        flat_set<fc::ecc::private_key> sign_keys;
        sign_keys.insert(u_9000_private_key);
        create_platform(u_9000_id, "platform", _core(10000), "www.123456789.com", "", sign_keys);
        create_license(u_9000_id, 6, "999999999", "license title", "license body", "extra", sign_keys);

        flat_set<fc::ecc::private_key> sign_keys1;
        sign_keys1.insert(u_1000_private_key);
        account_auth_platform(sign_keys1, u_1000_id, u_9000_id, 1000 * prec, account_statistics_object::Platform_Permission_Forward |
                                                                             account_statistics_object::Platform_Permission_Liked |
                                                                             account_statistics_object::Platform_Permission_Buyout |
                                                                             account_statistics_object::Platform_Permission_Comment |
                                                                             account_statistics_object::Platform_Permission_Reward);
        sign_keys1.insert(u_9000_private_key);

        map<account_uid_type, Recerptor_Parameter> receiptors;
        receiptors.insert(std::make_pair(u_9000_id, Recerptor_Parameter{ GRAPHENE_DEFAULT_PLATFORM_RECERPTS_RATIO, false, 0, 0}));
        receiptors.insert(std::make_pair(u_1000_id, Recerptor_Parameter{ 5000 , false, 0 , 0}));
        receiptors.insert(std::make_pair(u_2000_id, Recerptor_Parameter{ 2500, false, 0, 0 }));

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
        
        create_post(sign_keys1, u_9000_id, u_1000_id, "6666666", "document name", "document body", "extra", optional<account_uid_type>(), optional<account_uid_type>(), optional<post_pid_type>(), extension);

        const post_object& post = db.get_post_by_platform(u_9000_id, u_1000_id, 1);
        BOOST_CHECK(post.hash_value == "6666666");
        BOOST_CHECK(post.extra_data == "extra");
        BOOST_CHECK(post.title == "document name");
        BOOST_CHECK(post.body == "document body");
        BOOST_CHECK(post.forward_price == 10000 * prec);
        BOOST_CHECK(post.license_lid == 1);
        BOOST_CHECK(post.permission_flags == post_object::Post_Permission_Forward |
                                             post_object::Post_Permission_Liked |
                                             post_object::Post_Permission_Buyout |
                                             post_object::Post_Permission_Comment |
                                             post_object::Post_Permission_Reward);
        BOOST_CHECK(post.receiptors.find(u_9000_id) != post.receiptors.end());
        Recerptor_Parameter r9 = post.receiptors.find(u_9000_id)->second;
        BOOST_CHECK(r9 == Recerptor_Parameter(GRAPHENE_DEFAULT_PLATFORM_RECERPTS_RATIO, false, 0, 0));
        BOOST_CHECK(post.receiptors.find(u_1000_id) != post.receiptors.end());
        Recerptor_Parameter r1 = post.receiptors.find(u_1000_id)->second;
        BOOST_CHECK(r1 == Recerptor_Parameter(5000, false, 0, 0));
        BOOST_CHECK(post.receiptors.find(u_2000_id) != post.receiptors.end());
        Recerptor_Parameter r2 = post.receiptors.find(u_2000_id)->second;
        BOOST_CHECK(r2 == Recerptor_Parameter(2000, false, 0, 0));
        
    }
    catch (fc::exception& e) {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(comment_test)
{
    try{
        ACTORS((1000)(2000)(9000));
        account_manage(u_1000_id, account_manage_operation::opt{ true, true, true });
        account_manage(u_2000_id, account_manage_operation::opt{ true, true, true });

        const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
        auto _core = [&](int64_t x) -> asset
        {  return asset(x*prec);    };
        transfer(committee_account, u_1000_id, _core(10000));
        transfer(committee_account, u_2000_id, _core(10000));
        transfer(committee_account, u_9000_id, _core(10000));
        add_csaf_for_account(u_1000_id, 10000);
        add_csaf_for_account(u_2000_id, 10000);
        add_csaf_for_account(u_9000_id, 10000);

        flat_set<fc::ecc::private_key> sign_keys;
        sign_keys.insert(u_9000_private_key);
        create_platform(u_9000_id, "platform", _core(10000), "www.123456789.com", "", sign_keys);
        create_license(u_9000_id, 6, "999999999", "license title", "license body", "extra", sign_keys);

        flat_set<fc::ecc::private_key> sign_keys1;
        flat_set<fc::ecc::private_key> sign_keys2;
        sign_keys1.insert(u_1000_private_key);
        sign_keys2.insert(u_2000_private_key);
        account_auth_platform(sign_keys1, u_1000_id, u_9000_id, 1000 * prec, account_statistics_object::Platform_Permission_Forward |
                                                                             account_statistics_object::Platform_Permission_Liked |
                                                                             account_statistics_object::Platform_Permission_Buyout |
                                                                             account_statistics_object::Platform_Permission_Comment |
                                                                             account_statistics_object::Platform_Permission_Reward);
        account_auth_platform(sign_keys2, u_2000_id, u_9000_id, 1000 * prec, account_statistics_object::Platform_Permission_Forward |
                                                                             account_statistics_object::Platform_Permission_Liked |
                                                                             account_statistics_object::Platform_Permission_Buyout |
                                                                             account_statistics_object::Platform_Permission_Comment |
                                                                             account_statistics_object::Platform_Permission_Reward);
        sign_keys1.insert(u_9000_private_key);
        sign_keys2.insert(u_9000_private_key);

        post_operation::ext extension;
        extension.post_type = post_operation::Post_Type_Post;
        extension.forward_price = 10000 * prec;
        extension.license_lid = 1;
        extension.permission_flags = post_object::Post_Permission_Forward |
                                     post_object::Post_Permission_Liked |
                                     post_object::Post_Permission_Buyout |
                                     post_object::Post_Permission_Comment |
                                     post_object::Post_Permission_Reward;

        create_post(sign_keys1, u_9000_id, u_1000_id, "6666666", "document name", "document body", "extra", optional<account_uid_type>(), optional<account_uid_type>(), optional<post_pid_type>(), extension);

        extension.post_type = post_operation::Post_Type_Comment;
        create_post(sign_keys2, u_9000_id, u_2000_id, "2333333", "comment", "the post is good", "extra", u_9000_id, u_1000_id, 1, extension);
        const post_object& comment = db.get_post_by_platform(u_9000_id, u_2000_id, 1);
        BOOST_CHECK(comment.origin_platform  == u_9000_id);
        BOOST_CHECK(comment.origin_poster    == u_1000_id);
        BOOST_CHECK(comment.origin_post_pid  == 1);
        BOOST_CHECK(comment.hash_value       == "2333333");
        BOOST_CHECK(comment.title            == "comment");
        BOOST_CHECK(comment.body             == "the post is good");
        BOOST_CHECK(comment.extra_data       == "extra");
        BOOST_CHECK(comment.forward_price    == 10000 * prec);
        BOOST_CHECK(comment.license_lid      == 1);
        BOOST_CHECK(comment.permission_flags == post_object::Post_Permission_Forward |
                                                post_object::Post_Permission_Liked |
                                                post_object::Post_Permission_Buyout |
                                                post_object::Post_Permission_Comment |
                                                post_object::Post_Permission_Reward);
    }
    catch (fc::exception& e) {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(forward_test)
{
    try{
        ACTORS((1000)(2000)(9000)(9001));
        account_manage(u_1000_id, account_manage_operation::opt{ true, true, true });
        account_manage(u_2000_id, account_manage_operation::opt{ true, true, true });

        const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
        auto _core = [&](int64_t x) -> asset
        {  return asset(x*prec);    };
        transfer(committee_account, u_1000_id, _core(10000));
        transfer(committee_account, u_2000_id, _core(10000));
        transfer(committee_account, u_9000_id, _core(10000));
        transfer(committee_account, u_9001_id, _core(10000));
        add_csaf_for_account(u_1000_id, 10000);
        add_csaf_for_account(u_2000_id, 10000);
        add_csaf_for_account(u_9000_id, 10000);
        add_csaf_for_account(u_9001_id, 10000);
        transfer_extension({ u_1000_private_key }, u_1000_id, u_1000_id, _core(10000), "", true, false);
        transfer_extension({ u_2000_private_key }, u_2000_id, u_2000_id, _core(10000), "", true, false);
         
        flat_set<fc::ecc::private_key> sign_keys;
        sign_keys.insert(u_9000_private_key);
        create_platform(u_9000_id, "platform", _core(10000), "www.123456789.com", "", sign_keys);
        create_license(u_9000_id, 6, "999999999", "license title", "license body", "extra", sign_keys);
        flat_set<fc::ecc::private_key> sign_keys1;
        sign_keys1.insert(u_9001_private_key);
        create_platform(u_9001_id, "platform2", _core(10000), "www.655667669.com", "", sign_keys1);
        create_license(u_9001_id, 1, "7878787878", "license title", "license body", "extra", sign_keys1);


        flat_set<fc::ecc::private_key> sign_keys_1;
        flat_set<fc::ecc::private_key> sign_keys_2;
        sign_keys_1.insert(u_1000_private_key);
        sign_keys_2.insert(u_2000_private_key);
        account_auth_platform(sign_keys_1, u_1000_id, u_9000_id, 10000 * prec, account_statistics_object::Platform_Permission_Forward |
                                                                               account_statistics_object::Platform_Permission_Liked |
                                                                               account_statistics_object::Platform_Permission_Buyout |
                                                                               account_statistics_object::Platform_Permission_Comment |
                                                                               account_statistics_object::Platform_Permission_Reward);
        account_auth_platform(sign_keys_2, u_2000_id, u_9001_id, 10000 * prec, account_statistics_object::Platform_Permission_Forward |
                                                                               account_statistics_object::Platform_Permission_Liked |
                                                                               account_statistics_object::Platform_Permission_Buyout |
                                                                               account_statistics_object::Platform_Permission_Comment |
                                                                               account_statistics_object::Platform_Permission_Reward);
        sign_keys_1.insert(u_9000_private_key);
        sign_keys_2.insert(u_9001_private_key);
        bool do_by_platform = true; // modify to false , change to do_by_account
        if (do_by_platform){
            sign_keys_2.erase(u_2000_private_key);
        }

        post_operation::ext extension;
        extension.post_type = post_operation::Post_Type_Post;
        extension.forward_price = 10000 * prec;
        extension.license_lid = 1;
        extension.permission_flags = post_object::Post_Permission_Forward |
                                     post_object::Post_Permission_Liked |
                                     post_object::Post_Permission_Buyout |
                                     post_object::Post_Permission_Comment |
                                     post_object::Post_Permission_Reward;

        create_post(sign_keys_1, u_9000_id, u_1000_id, "6666666", "document name", "document body", "extra", optional<account_uid_type>(), optional<account_uid_type>(), optional<post_pid_type>(), extension);

        extension.post_type = post_operation::Post_Type_forward_And_Modify;
        create_post(sign_keys_2, u_9001_id, u_2000_id, "9999999", "new titile", "new body", "extra", u_9000_id, u_1000_id, 1, extension);

        const post_object& forward_post = db.get_post_by_platform(u_9001_id, u_2000_id, 1);
        BOOST_CHECK(forward_post.origin_platform  == u_9000_id);
        BOOST_CHECK(forward_post.origin_poster    == u_1000_id);
        BOOST_CHECK(forward_post.origin_post_pid  == 1);
        BOOST_CHECK(forward_post.hash_value       == "9999999");
        BOOST_CHECK(forward_post.title            == "new titile");
        BOOST_CHECK(forward_post.body             == "new body");
        BOOST_CHECK(forward_post.extra_data       == "extra");
        BOOST_CHECK(forward_post.forward_price    == 10000 * prec);
        BOOST_CHECK(forward_post.license_lid      == 1);
        BOOST_CHECK(forward_post.permission_flags == post_object::Post_Permission_Forward |
                                                     post_object::Post_Permission_Liked |
                                                     post_object::Post_Permission_Buyout |
                                                     post_object::Post_Permission_Comment |
                                                     post_object::Post_Permission_Reward);

        const account_statistics_object& sobj1 = db.get_account_statistics_by_uid(u_1000_id);
        BOOST_CHECK(sobj1.prepaid == 17000 * prec);
        const account_statistics_object& platform1 = db.get_account_statistics_by_uid(u_9000_id);
        BOOST_CHECK(platform1.prepaid      == 3000 * prec);
        BOOST_CHECK(platform1.core_balance == 10000 * prec);
        const account_statistics_object& sobj2 = db.get_account_statistics_by_uid(u_2000_id);
        BOOST_CHECK(sobj2.prepaid == 0);
        
        if (do_by_platform){
            auto auth_data = sobj2.prepaids_for_platform.find(u_9001_id);
            BOOST_CHECK(auth_data != sobj2.prepaids_for_platform.end());
            BOOST_CHECK(auth_data->second.cur_used == 10000*prec);
            BOOST_CHECK(sobj2.get_auth_platform_usable_prepaid(u_9001_id) == 0);
        }
    }
    catch (fc::exception& e) {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(buyout_test)
{
    try{
        ACTORS((1000)(2000)(9000));
        account_manage(u_1000_id, account_manage_operation::opt{ true, true, true });
        account_manage(u_2000_id, account_manage_operation::opt{ true, true, true });

        const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
        auto _core = [&](int64_t x) -> asset
        {  return asset(x*prec);    };
        transfer(committee_account, u_1000_id, _core(10000));
        transfer(committee_account, u_2000_id, _core(10000));
        transfer(committee_account, u_9000_id, _core(10000));
        add_csaf_for_account(u_1000_id, 10000);
        add_csaf_for_account(u_2000_id, 10000);
        add_csaf_for_account(u_9000_id, 10000);
        transfer_extension({ u_1000_private_key }, u_1000_id, u_1000_id, _core(10000), "", true, false);
        transfer_extension({ u_2000_private_key }, u_2000_id, u_2000_id, _core(10000), "", true, false);

        flat_set<fc::ecc::private_key> sign_keys;
        sign_keys.insert(u_9000_private_key);
        create_platform(u_9000_id, "platform", _core(10000), "www.123456789.com", "", sign_keys);
        create_license(u_9000_id, 6, "999999999", "license title", "license body", "extra", sign_keys);


        flat_set<fc::ecc::private_key> sign_keys_1;
        flat_set<fc::ecc::private_key> sign_keys_2;
        sign_keys_1.insert(u_1000_private_key);
        sign_keys_2.insert(u_2000_private_key);
        account_auth_platform(sign_keys_1, u_1000_id, u_9000_id, 10000 * prec, account_statistics_object::Platform_Permission_Forward |
                                                                               account_statistics_object::Platform_Permission_Liked |
                                                                               account_statistics_object::Platform_Permission_Buyout |
                                                                               account_statistics_object::Platform_Permission_Comment |
                                                                               account_statistics_object::Platform_Permission_Reward);
        account_auth_platform(sign_keys_2, u_2000_id, u_9000_id, 10000 * prec, account_statistics_object::Platform_Permission_Forward |
                                                                               account_statistics_object::Platform_Permission_Liked |
                                                                               account_statistics_object::Platform_Permission_Buyout |
                                                                               account_statistics_object::Platform_Permission_Comment |
                                                                               account_statistics_object::Platform_Permission_Reward);
        sign_keys_1.insert(u_9000_private_key);
        sign_keys_2.insert(u_9000_private_key);
        bool do_by_platform = true; // modify to false , change to do_by_account
        if (do_by_platform){
            sign_keys_2.erase(u_2000_private_key);
        }

        post_operation::ext extension;
        extension.post_type = post_operation::Post_Type_Post;
        extension.forward_price = 10000 * prec;
        extension.license_lid = 1;
        extension.permission_flags = post_object::Post_Permission_Forward |
                                     post_object::Post_Permission_Liked |
                                     post_object::Post_Permission_Buyout |
                                     post_object::Post_Permission_Comment |
                                     post_object::Post_Permission_Reward;

        create_post(sign_keys_1, u_9000_id, u_1000_id, "6666666", "document name", "document body", "extra", optional<account_uid_type>(), optional<account_uid_type>(), optional<post_pid_type>(), extension);

        post_update_operation::ext ext;
        ext.receiptor = u_1000_id;
        ext.to_buyout = true;
        ext.buyout_ratio = 3000;
        ext.buyout_price = 1000 * prec;
        ext.buyout_expiration = time_point_sec::maximum();
        update_post({ u_1000_private_key, u_9000_private_key }, u_9000_id, u_1000_id, 1, "", "", "", "", ext);

        buyout_post(u_2000_id, u_9000_id, u_1000_id, 1, u_1000_id, sign_keys_2);

        const post_object& post = db.get_post_by_platform(u_9000_id, u_1000_id, 1);
        auto iter1 = post.receiptors.find(u_1000_id);
        BOOST_CHECK(iter1 != post.receiptors.end());
        BOOST_CHECK(iter1->second.cur_ratio == 4000);
        BOOST_CHECK(iter1->second.to_buyout == false);
        BOOST_CHECK(iter1->second.buyout_ratio == 0);
        BOOST_CHECK(iter1->second.buyout_price == 0);

        auto iter2 = post.receiptors.find(u_2000_id);
        BOOST_CHECK(iter2 != post.receiptors.end());
        BOOST_CHECK(iter2->second.cur_ratio == 3000);
        BOOST_CHECK(iter2->second.to_buyout == false);
        BOOST_CHECK(iter2->second.buyout_ratio == 0);
        BOOST_CHECK(iter2->second.buyout_price == 0);

        const account_statistics_object& sobj1 = db.get_account_statistics_by_uid(u_1000_id);
        BOOST_CHECK(sobj1.prepaid == 11000 * prec);
        const account_statistics_object& sobj2 = db.get_account_statistics_by_uid(u_2000_id);
        BOOST_CHECK(sobj2.prepaid == 9000 * prec);

        if (do_by_platform){
            auto auth_data = sobj2.prepaids_for_platform.find(u_9000_id);
            BOOST_CHECK(auth_data != sobj2.prepaids_for_platform.end());
            BOOST_CHECK(auth_data->second.cur_used == 1000 * prec);
        }
    }
    catch (fc::exception& e) {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_SUITE_END()
