
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

#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE( operation_tests, database_fixture )


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

BOOST_AUTO_TEST_CASE(transfer_extension_test)
{
    try{
        ACTORS((1000)(1001));

        const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);

        // Return number of core shares (times precision)
        auto _core = [&](int64_t x) -> asset
        {  return asset(x*prec);    };

        transfer(committee_account, u_1000_id, _core(10000));
        transfer(committee_account, u_1001_id, _core(10000));
        add_csaf_for_account(u_1000_id, 10000);
        add_csaf_for_account(u_1001_id, 10000);
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
                                                                             account_statistics_object::Platform_Permission_Reward);

        const account_statistics_object& ant1000 = db.get_account_statistics_by_uid(u_1000_id);
        auto iter = ant1000.prepaids_for_platform.find(u_9000_id);
        BOOST_CHECK(iter != ant1000.prepaids_for_platform.end());
        BOOST_CHECK(iter->second.max_limit == 1000 * prec);
        BOOST_CHECK(iter->second.permission_flags & account_statistics_object::Platform_Permission_Forward);
        BOOST_CHECK(iter->second.permission_flags & account_statistics_object::Platform_Permission_Liked);
        BOOST_CHECK(iter->second.permission_flags & account_statistics_object::Platform_Permission_Buyout);
        BOOST_CHECK(iter->second.permission_flags & account_statistics_object::Platform_Permission_Comment);
        BOOST_CHECK(iter->second.permission_flags & account_statistics_object::Platform_Permission_Reward);


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
        BOOST_CHECK(license.hash_value == "999999999");
        BOOST_CHECK(license.extra_data == "extra");
        BOOST_CHECK(license.title == "license title");
        BOOST_CHECK(license.body == "license body");
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
        receiptors.insert(std::make_pair(u_2000_id, Recerptor_Parameter{ 2000, false, 0, 0 }));

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

BOOST_AUTO_TEST_SUITE_END()
