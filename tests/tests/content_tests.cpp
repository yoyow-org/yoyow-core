
#include <boost/test/unit_test.hpp>

#include <graphene/chain/database.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>

#include <graphene/chain/protocol/content.hpp>
#include <graphene/chain/content_object.hpp>
#include <graphene/chain/pledge_mining_object.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/advertising_object.hpp>
#include <graphene/chain/custom_vote_object.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/log/logger.hpp>

#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE( operation_tests, database_fixture )

BOOST_AUTO_TEST_CASE(update_reward_percent_test)
{
   try
   {
      ACTORS((1000));
      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };

      transfer(committee_account, u_1000_id, _core(100000000));
      collect_csaf_from_committee(u_1000_id, 100);
      generate_blocks(HARDFORK_0_5_TIME, true);

      auto market_fee_percent = 1 * GRAPHENE_1_PERCENT;
      auto market_reward_percent = 50 * GRAPHENE_1_PERCENT;
      auto max_supply = 100000000 * prec;
      auto max_market_fee_1 = 20 * prec;
      auto max_market_fee_2 = 2000 * prec;
      asset_options options;
      options.max_supply = max_supply;
      options.market_fee_percent = market_fee_percent;
      options.max_market_fee = max_market_fee_1;
      options.issuer_permissions = 15;
      options.flags = charge_market_fee;
      options.description = "test asset";
      options.extensions = graphene::chain::extension<additional_asset_options>();
      additional_asset_options exts;
      exts.reward_percent = market_reward_percent;
      options.extensions->value = exts;

      const account_object commit_obj = db.get_account_by_uid(committee_account);

      auto initial_supply = share_type(100000000 * prec);

      create_asset({ u_1000_private_key }, u_1000_id, "ABC", 5, options, initial_supply);
      const asset_object& ast = db.get_asset_by_aid(1);
      BOOST_CHECK(ast.symbol == "ABC");
      BOOST_CHECK(ast.precision == 5);
      BOOST_CHECK(ast.issuer == u_1000_id);
      BOOST_CHECK(ast.options.max_supply == max_supply);
      BOOST_CHECK(ast.options.flags == charge_market_fee);
      asset ast1 = db.get_balance(u_1000_id, 1);
      BOOST_CHECK(ast1.asset_id == 1);
      BOOST_CHECK(ast1.amount == initial_supply);

      exts.reward_percent = market_reward_percent/2;
      options.extensions->value = exts;
      update_asset({ u_1000_private_key }, u_1000_id, 1, optional<uint8_t>(), options);
      const asset_object& ast2 = db.get_asset_by_aid(1);
      BOOST_CHECK(ast2.options.extensions.valid());
      BOOST_CHECK(ast2.options.extensions->value.reward_percent.valid());
      BOOST_CHECK(*(ast2.options.extensions->value.reward_percent) == market_reward_percent / 2);
   }
   catch (const fc::exception& e)
   {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(witness_csaf_test)
{
   try
   {
      ACTORS((1000)(2000));
      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };

      transfer(committee_account, u_1000_id, _core(100000000));
      transfer(committee_account, u_2000_id, _core(100000000));
      collect_csaf_from_committee(u_1000_id, 100);
      collect_csaf_from_committee(u_2000_id, 100);
      
      //###############################  before reduce witness csaf on hardfork_4_time
      const witness_object& witness1 = create_witness(u_1000_id, u_1000_private_key, _core(100000));
      generate_block(10);
      collect_csaf_origin({ u_1000_private_key }, u_1000_id, u_1000_id, 1);
      const _account_statistics_object& ants1_1000 = db.get_account_statistics_by_uid(u_1000_id);
      time_point_sec now1 = db.head_block_time();
      fc::time_point_sec now_rounded1((now1.sec_since_epoch() / 60) * 60);
      share_type effective_balance1 = ants1_1000.core_balance + ants1_1000.core_leased_in - ants1_1000.core_leased_out;
      auto coin_seconds_earned1 = compute_coin_seconds_earned(ants1_1000, effective_balance1, now_rounded1);
      BOOST_CHECK(ants1_1000.coin_seconds_earned == coin_seconds_earned1);
      

      generate_blocks(HARDFORK_0_4_TIME, true);
      //###############################  update witness csaf on hardfork_4_time
      const _account_statistics_object& ants2_1000 = db.get_account_statistics_by_uid(u_1000_id);
      time_point_sec now2 = db.head_block_time();
      fc::time_point_sec now_rounded2((now2.sec_since_epoch() / 60) * 60);
      share_type effective_balance2 = ants2_1000.core_balance + ants2_1000.core_leased_in - ants2_1000.core_leased_out;
      auto coin_seconds_earned2 = compute_coin_seconds_earned(ants2_1000, effective_balance2, now_rounded2);
      BOOST_CHECK(ants2_1000.coin_seconds_earned == coin_seconds_earned2);

      generate_block(10);
      //###############################  reduce witness pledge for computing csaf after hardfork_4_time
      collect_csaf_origin({ u_1000_private_key }, u_1000_id, u_1000_id, 1);
      const _account_statistics_object& ants3_1000 = db.get_account_statistics_by_uid(u_1000_id);
      time_point_sec now3 = db.head_block_time();
      fc::time_point_sec now_rounded3((now3.sec_since_epoch() / 60) * 60);
      share_type effective_balance3 = ants3_1000.core_balance + ants3_1000.core_leased_in - ants3_1000.core_leased_out
         - ants3_1000.get_pledge_balance(GRAPHENE_CORE_ASSET_AID, pledge_balance_type::Witness, db);
      auto coin_seconds_earned3 = compute_coin_seconds_earned(ants3_1000, effective_balance3, now_rounded3);
      BOOST_CHECK(ants3_1000.coin_seconds_earned == coin_seconds_earned3);

      generate_blocks(HARDFORK_0_5_TIME, true);
      //After hardfork_05_time , csaf compute only depends on locked_balance for fee
      const witness_object& witness2 = create_witness(u_2000_id, u_2000_private_key, _core(10000));
      balance_lock_update({ u_2000_private_key }, u_2000_id, 10000 * prec);
      collect_csaf_origin({ u_2000_private_key }, u_2000_id, u_2000_id, 1);
      const _account_statistics_object& ants_2000 = db.get_account_statistics_by_uid(u_2000_id);
      time_point_sec now4 = db.head_block_time();
      fc::time_point_sec now_rounded4((now4.sec_since_epoch() / 60) * 60);
      share_type effective_balance4;
      auto pledge_balance_obj = ants_2000.pledge_balance_ids.at(pledge_balance_type::Lock_balance)(db);
      if (GRAPHENE_CORE_ASSET_AID == pledge_balance_obj.asset_id)
         effective_balance4 = pledge_balance_obj.pledge;
      auto coin_seconds_earned4 = compute_coin_seconds_earned(ants_2000, effective_balance4, now_rounded4);

      BOOST_CHECK(ants_2000.coin_seconds_earned == coin_seconds_earned4);

   }
   catch (const fc::exception& e)
   {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(collect_csaf_test)
{
   try
   {
      ACTORS((1000)(2000));
      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);

      collect_csaf({ u_1000_private_key }, u_1000_id, u_1000_id, 1000);

      const _account_statistics_object& ants_1000 = db.get_account_statistics_by_uid(u_1000_id);
      BOOST_CHECK(ants_1000.csaf == 1000 * prec);

      //###############################################################
      collect_csaf_from_committee(u_2000_id, 1000);
      const _account_statistics_object& ants_2000 = db.get_account_statistics_by_uid(u_2000_id);
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
      //generate_blocks(HARDFORK_0_5_TIME, true);
      for (int i = 0; i < 5; ++i)
         add_csaf_for_account(genesis_state.initial_accounts.at(i).uid, 1000);

      generate_blocks(10);

      committee_update_global_extension_parameter_item_type item;
      item.value = { 300, 300, 1000, 31536000, 10, 10000000000000, 10000000000000, 10000000000000, 1, 100,
         3000, 30000, 62000000, 4000, 2000, 8000, 9000, 11000, 20, 2000000, 3640000 };

      item.value.min_witness_block_produce_pledge = 60000000000;
      //item.content_award_skip_slots;
      item.value.unlocked_balance_release_delay = 28800 * 8;
      item.value.min_mining_pledge = 200000000;
      item.value.mining_pledge_release_delay = 28800 * 8;
      item.value.max_pledge_mining_bonus_rate = 8000;
      item.value.registrar_referrer_rate_from_score = 1000;
      item.value.max_pledge_releasing_size = 30;

      committee_proposal_create(genesis_state.initial_accounts.at(0).uid, { item }, 100, voting_opinion_type::opinion_for, 100, 100);
      for (int i = 1; i < 5; ++i)
         committee_proposal_vote(genesis_state.initial_accounts.at(i).uid, 1, voting_opinion_type::opinion_for);

      generate_blocks(101);
      auto gap = db.get_global_properties().parameters.get_extension_params();

      BOOST_REQUIRE_EQUAL(gap.content_award_interval, 300);
      BOOST_REQUIRE_EQUAL(gap.platform_award_interval, 300);
      BOOST_REQUIRE_EQUAL(gap.max_csaf_per_approval.value, 1000);
      BOOST_REQUIRE_EQUAL(gap.approval_expiration, 31536000);
      BOOST_REQUIRE_EQUAL(gap.min_effective_csaf.value, 10);
      BOOST_REQUIRE_EQUAL(gap.total_content_award_amount.value, 10000000000000);
      BOOST_REQUIRE_EQUAL(gap.total_platform_content_award_amount.value, 10000000000000);
      BOOST_REQUIRE_EQUAL(gap.total_platform_voted_award_amount.value, 10000000000000);
      BOOST_REQUIRE_EQUAL(gap.platform_award_min_votes.value, 1);
      BOOST_REQUIRE_EQUAL(gap.platform_award_requested_rank, 100);

      BOOST_REQUIRE_EQUAL(gap.platform_award_basic_rate, 3000);
      BOOST_REQUIRE_EQUAL(gap.casf_modulus, 30000);
      BOOST_REQUIRE_EQUAL(gap.post_award_expiration, 62000000);
      BOOST_REQUIRE_EQUAL(gap.approval_casf_min_weight, 4000);
      BOOST_REQUIRE_EQUAL(gap.approval_casf_first_rate, 2000);
      BOOST_REQUIRE_EQUAL(gap.approval_casf_second_rate, 8000);
      BOOST_REQUIRE_EQUAL(gap.receiptor_award_modulus, 9000);
      BOOST_REQUIRE_EQUAL(gap.disapprove_award_modulus, 11000);

      BOOST_REQUIRE_EQUAL(gap.advertising_confirmed_fee_rate, 20);
      BOOST_REQUIRE_EQUAL(gap.advertising_confirmed_min_fee.value, 2000000);
      BOOST_REQUIRE_EQUAL(gap.custom_vote_effective_time, 3640000);

      BOOST_REQUIRE_EQUAL(gap.min_witness_block_produce_pledge, 60000000000);
      BOOST_REQUIRE_EQUAL(gap.unlocked_balance_release_delay, 28800 * 8);
      BOOST_REQUIRE_EQUAL(gap.min_mining_pledge, 200000000);
      BOOST_REQUIRE_EQUAL(gap.mining_pledge_release_delay, 28800 * 8);
      BOOST_REQUIRE_EQUAL(gap.max_pledge_mining_bonus_rate, 8000);
      BOOST_REQUIRE_EQUAL(gap.registrar_referrer_rate_from_score, 1000);
      BOOST_REQUIRE_EQUAL(gap.max_pledge_releasing_size, 30);
   }
   catch (const fc::exception& e)
   {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(committee_proposal_threshold_test)
{
   try
   {
      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);

      // Return number of core shares (times precision)
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };

      // make sure the database requires our fee to be nonzero
      enable_fees();
      generate_blocks(HARDFORK_0_5_TIME, true);
      for (int i = 0; i < 5; ++i)
         add_csaf_for_account(genesis_state.initial_accounts.at(i).uid, 1000);

      committee_update_global_extension_parameter_item_type item;
      item.value = { 300 };

      //not enough threshold
      committee_proposal_create(genesis_state.initial_accounts.at(0).uid, { item }, 100, voting_opinion_type::opinion_for, 100, 100);
      generate_blocks(101);
      auto gap = db.get_global_properties().parameters.get_extension_params();
      BOOST_REQUIRE_EQUAL(gap.content_award_interval, 0);

      //enough threshold
      committee_proposal_create(genesis_state.initial_accounts.at(0).uid, { item }, 200, voting_opinion_type::opinion_for, 200, 200);
      for (int i = 1; i < 5; ++i)
         committee_proposal_vote(genesis_state.initial_accounts.at(i).uid, 2, voting_opinion_type::opinion_for);
      generate_blocks(101);
      auto gap2 = db.get_global_properties().parameters.get_extension_params();
      BOOST_REQUIRE_EQUAL(gap2.content_award_interval, 300);
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

      transfer(committee_account, u_1001_id, _core(100000));
      transfer(committee_account, u_9000_id, _core(100000));

      add_csaf_for_account(u_1001_id, 10000);
      add_csaf_for_account(u_9000_id, 10000);
      generate_blocks(HARDFORK_0_4_TIME, true);

      create_platform(u_9000_id, "platform", _core(10000), "www.123456789.com", "", { u_9000_private_key });
      account_auth_platform({ u_1001_private_key }, u_1001_id, u_9000_id, 1000 * prec, account_auth_platform_object::Platform_Permission_Forward |
         account_auth_platform_object::Platform_Permission_Liked |
         account_auth_platform_object::Platform_Permission_Buyout |
         account_auth_platform_object::Platform_Permission_Comment |
         account_auth_platform_object::Platform_Permission_Reward |
         account_auth_platform_object::Platform_Permission_Post |
         account_auth_platform_object::Platform_Permission_Content_Update);
      create_license(u_9000_id, 6, "999999999", "license title", "license body", "extra", { u_9000_private_key });

      post_operation::ext extensions;
      extensions.license_lid = 1;
      create_post({ u_1001_private_key, u_9000_private_key }, u_9000_id, u_1001_id, "", "", "", "",
         optional<account_uid_type>(),
         optional<account_uid_type>(),
         optional<post_pid_type>(),
         extensions);

      post_update_operation::ext ext;
      ext.forward_price = 100 * prec;
      ext.receiptor = u_1001_id;
      ext.to_buyout = true;
      ext.buyout_ratio = 3000;
      ext.buyout_price = 10000 * prec;
      ext.license_lid = 1;
      ext.permission_flags = 0xF;
      update_post({ u_1001_private_key, u_9000_private_key }, u_9000_id, u_1001_id, 1, "", "", "", "", ext);

      auto post_obj = db.get_post_by_platform(u_9000_id, u_1001_id, 1);
      Receiptor_Parameter parameter = post_obj.receiptors[u_1001_id];

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
      committee_update_global_extension_parameter_item_type item;
      item.value = { 300, 300, 1000, 31536000, 10, 10000000000000, 10000000000000, 10000000000000, 1000, 100 };
      committee_proposal_create(genesis_state.initial_accounts.at(0).uid, { item }, 100, voting_opinion_type::opinion_for, 100, 100);
      for (int i = 1; i < 5; ++i)
         committee_proposal_vote(genesis_state.initial_accounts.at(i).uid, 1, voting_opinion_type::opinion_for);
      generate_blocks(89);

      for (auto a : score_map)
         add_csaf_for_account(a.first, 10000);
      add_csaf_for_account(u_9000_id, 10000);
      generate_blocks(HARDFORK_0_4_TIME, true);

      create_platform(u_9000_id, "platform", _core(10000), "www.123456789.com", "", { u_9000_private_key });
      create_license(u_9000_id, 6, "999999999", "license title", "license body", "extra", { u_9000_private_key });
      account_auth_platform({ u_1001_private_key }, u_1001_id, u_9000_id, 10000 * prec, account_auth_platform_object::Platform_Permission_Forward |
         account_auth_platform_object::Platform_Permission_Liked |
         account_auth_platform_object::Platform_Permission_Buyout |
         account_auth_platform_object::Platform_Permission_Comment |
         account_auth_platform_object::Platform_Permission_Reward |
         account_auth_platform_object::Platform_Permission_Post |
         account_auth_platform_object::Platform_Permission_Content_Update);

      post_operation::ext extensions;
      extensions.license_lid = 1;
      create_post({ u_1001_private_key, u_9000_private_key }, u_9000_id, u_1001_id, "", "", "", "",
         optional<account_uid_type>(),
         optional<account_uid_type>(),
         optional<post_pid_type>(),
         extensions);

      for (auto a : score_map)
      {
         account_auth_platform({ a.second }, a.first, u_9000_id, 1000 * prec, 0x1F);
         account_manage(a.first, { true, true, true });
         score_a_post({ a.second }, a.first, u_9000_id, u_1001_id, 1, 5, 10);
      }

      auto dpo = db.get_dynamic_global_properties();
      const auto& apt_idx = db.get_index_type<active_post_index>().indices().get<by_post_pid>();
      auto apt_itr = apt_idx.find(std::make_tuple(u_9000_id, u_1001_id, dpo.current_active_post_sequence, 1));
      BOOST_CHECK(apt_itr != apt_idx.end());
      auto active_post = *apt_itr;
      BOOST_CHECK(active_post.total_csaf == 10 * 10);

      for (auto a : score_map)
      {
         auto score_obj = db.get_score(u_9000_id, u_1001_id, 1, a.first);
         BOOST_CHECK(score_obj.score == 5);
         BOOST_CHECK(score_obj.csaf == 10);
      }

      //test clear expired score
      generate_blocks(10);
      generate_block(~0, generate_private_key("null_key"), 10512000);
      for (auto a : score_map)
      {
         auto score_obj = db.find_score(u_9000_id, u_1001_id, 1, a.first);
         BOOST_CHECK(score_obj == nullptr);
      }
   }
   catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(reward_test)
{
   try{
      ACTORS((1001)(9000));

      flat_map<account_uid_type, fc::ecc::private_key> reward_map;
      actor(1003, 10, reward_map);
      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);

      // Return number of core shares (times precision)
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };

      transfer(committee_account, u_9000_id, _core(100000));
      generate_blocks(10);

      BOOST_TEST_MESSAGE("Turn on the reward mechanism, open content award and platform voted award");
      committee_update_global_extension_parameter_item_type item;
      item.value = { 300, 300, 1000, 31536000, 10, 10000000000000, 10000000000000, 10000000000000, 1000, 100 };
      committee_proposal_create(genesis_state.initial_accounts.at(0).uid, { item }, 100, voting_opinion_type::opinion_for, 100, 100);
      for (int i = 1; i < 5; ++i)
         committee_proposal_vote(genesis_state.initial_accounts.at(i).uid, 1, voting_opinion_type::opinion_for);
      generate_blocks(89);

      for (auto a : reward_map)
         add_csaf_for_account(a.first, 10000);
      add_csaf_for_account(u_9000_id, 10000);


      generate_blocks(HARDFORK_0_4_TIME,true);
      create_platform(u_9000_id, "platform", _core(10000), "www.123456789.com", "", { u_9000_private_key });
      create_license(u_9000_id, 6, "999999999", "license title", "license body", "extra", { u_9000_private_key });
      account_auth_platform({ u_1001_private_key }, u_1001_id, u_9000_id, 10000 * prec, account_auth_platform_object::Platform_Permission_Forward |
         account_auth_platform_object::Platform_Permission_Liked |
         account_auth_platform_object::Platform_Permission_Buyout |
         account_auth_platform_object::Platform_Permission_Comment |
         account_auth_platform_object::Platform_Permission_Reward |
         account_auth_platform_object::Platform_Permission_Post |
         account_auth_platform_object::Platform_Permission_Content_Update);

      post_operation::ext extensions;
      extensions.license_lid = 1;

      create_post({ u_1001_private_key, u_9000_private_key }, u_9000_id, u_1001_id, "", "", "", "",
         optional<account_uid_type>(),
         optional<account_uid_type>(),
         optional<post_pid_type>(),
         extensions);

      for (auto a : reward_map)
      {
         transfer(committee_account, a.first, _core(100000));
         reward_post(a.first, u_9000_id, u_1001_id, 1, _core(1000), { a.second });
      }

      auto dpo = db.get_dynamic_global_properties();
      const auto& apt_idx = db.get_index_type<active_post_index>().indices().get<by_post_pid>();
      auto apt_itr = apt_idx.find(std::make_tuple(u_9000_id, u_1001_id, dpo.current_active_post_sequence, 1));
      BOOST_CHECK(apt_itr != apt_idx.end());
      auto active_post = *apt_itr;
      BOOST_CHECK(active_post.total_rewards.find(GRAPHENE_CORE_ASSET_AID) != active_post.total_rewards.end());
      BOOST_CHECK(active_post.total_rewards[GRAPHENE_CORE_ASSET_AID] == 10 * 1000 * prec);

      BOOST_CHECK(active_post.receiptor_details.find(u_9000_id) != active_post.receiptor_details.end());
      auto iter_reward = active_post.receiptor_details[u_9000_id].rewards.find(GRAPHENE_CORE_ASSET_AID);
      BOOST_CHECK(iter_reward != active_post.receiptor_details[u_9000_id].rewards.end());
      BOOST_CHECK(iter_reward->second == 10 * 250 * prec);

      BOOST_CHECK(active_post.receiptor_details.find(u_1001_id) != active_post.receiptor_details.end());
      auto iter_reward2 = active_post.receiptor_details[u_1001_id].rewards.find(GRAPHENE_CORE_ASSET_AID);
      BOOST_CHECK(iter_reward2 != active_post.receiptor_details[u_1001_id].rewards.end());
      BOOST_CHECK(iter_reward2->second == 10 * 750 * prec);

      const platform_object& platform = db.get_platform_by_owner(u_9000_id);
      auto iter_profit = platform.period_profits.find(dpo.current_active_post_sequence);
      BOOST_CHECK(iter_profit != platform.period_profits.end());
      auto iter_reward_profit = iter_profit->second.rewards_profits.find(GRAPHENE_CORE_ASSET_AID);
      BOOST_CHECK(iter_reward_profit != iter_profit->second.rewards_profits.end());
      BOOST_CHECK(iter_reward_profit->second == 10 * 250 * prec);

      post_object post_obj = db.get_post_by_platform(u_9000_id, u_1001_id, 1);
      int64_t poster_earned = (post_obj.receiptors[u_1001_id].cur_ratio * uint128_t(100000000) / 10000).convert_to<int64_t>();
      int64_t platform_earned = 100000000 - poster_earned;

      auto act_1001 = db.get_account_statistics_by_uid(u_1001_id);
      BOOST_CHECK(act_1001.core_balance == poster_earned * 10);
      auto act_9000 = db.get_account_statistics_by_uid(u_9000_id);
      BOOST_CHECK(act_9000.core_balance == (platform_earned * 10 + 100000 * prec));

      for (auto a : reward_map)
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
      ACTORS((1001)(1002)(1003)(9000));

      flat_map<account_uid_type, fc::ecc::private_key> score_map1;
      flat_map<account_uid_type, fc::ecc::private_key> score_map2;
      flat_map<account_uid_type, fc::ecc::private_key> score_map3;
      flat_map<account_uid_type, fc::ecc::private_key> score_map4;
      flat_map<account_uid_type, fc::ecc::private_key> score_map5;
      actor(1005, 20, score_map1);
      actor(2005, 20, score_map2);
      actor(3005, 20, score_map3);
      actor(4005, 20, score_map4);
      actor(5005, 20, score_map5);

      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);

      // Return number of core shares (times precision)
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };

      for (int i = 0; i < 5; ++i)
         add_csaf_for_account(genesis_state.initial_accounts.at(i).uid, 1000);
      transfer(committee_account, u_9000_id, _core(100000));
      //to remain_budget_pool can reward enough
      generate_blocks(200000);

      BOOST_TEST_MESSAGE("Turn on the reward mechanism, open content award and platform voted award");
      committee_update_global_extension_parameter_item_type item;
      item.value = { 300, 300, 1000, 31536000, 10, 10000000000, 10000000000, 10000000000, 1000, 100 };
      auto execute_proposal_head_block = db.head_block_num() + 100;
      committee_proposal_create(genesis_state.initial_accounts.at(0).uid, { item }, execute_proposal_head_block, voting_opinion_type::opinion_for, execute_proposal_head_block, execute_proposal_head_block);
      for (int i = 1; i < 5; ++i)
         committee_proposal_vote(genesis_state.initial_accounts.at(i).uid, 1, voting_opinion_type::opinion_for);
      generate_blocks(102);
      generate_blocks(HARDFORK_0_5_TIME, true);

      collect_csaf_from_committee(u_9000_id, 1000);
      create_platform(u_9000_id, "platform", _core(10000), "www.123456789.com", "", { u_9000_private_key });
      create_license(u_9000_id, 6, "999999999", "license title", "license body", "extra", { u_9000_private_key });
      account_auth_platform({ u_1001_private_key }, u_1001_id, u_9000_id, 10000 * prec, account_auth_platform_object::Platform_Permission_Forward |
         account_auth_platform_object::Platform_Permission_Liked |
         account_auth_platform_object::Platform_Permission_Buyout |
         account_auth_platform_object::Platform_Permission_Comment |
         account_auth_platform_object::Platform_Permission_Reward |
         account_auth_platform_object::Platform_Permission_Post |
         account_auth_platform_object::Platform_Permission_Content_Update);
      account_auth_platform({ u_1002_private_key }, u_1002_id, u_9000_id, 10000 * prec, account_auth_platform_object::Platform_Permission_Forward |
         account_auth_platform_object::Platform_Permission_Liked |
         account_auth_platform_object::Platform_Permission_Buyout |
         account_auth_platform_object::Platform_Permission_Comment |
         account_auth_platform_object::Platform_Permission_Reward |
         account_auth_platform_object::Platform_Permission_Post |
         account_auth_platform_object::Platform_Permission_Content_Update);
      account_auth_platform({ u_1003_private_key }, u_1003_id, u_9000_id, 10000 * prec, account_auth_platform_object::Platform_Permission_Forward |
         account_auth_platform_object::Platform_Permission_Liked |
         account_auth_platform_object::Platform_Permission_Buyout |
         account_auth_platform_object::Platform_Permission_Comment |
         account_auth_platform_object::Platform_Permission_Reward |
         account_auth_platform_object::Platform_Permission_Post |
         account_auth_platform_object::Platform_Permission_Content_Update);

      post_operation::ext extensions;
      extensions.license_lid = 1;
      create_post({ u_1001_private_key, u_9000_private_key }, u_9000_id, u_1001_id, "", "", "", "",
         optional<account_uid_type>(),
         optional<account_uid_type>(),
         optional<post_pid_type>(),
         extensions);
      create_post({ u_1002_private_key, u_9000_private_key }, u_9000_id, u_1002_id, "", "", "", "",
         optional<account_uid_type>(),
         optional<account_uid_type>(),
         optional<post_pid_type>(),
         extensions);
      create_post({ u_1003_private_key, u_9000_private_key }, u_9000_id, u_1003_id, "", "", "", "",
         optional<account_uid_type>(),
         optional<account_uid_type>(),
         optional<post_pid_type>(),
         extensions);

      account_manage_operation::opt options;
      options.can_rate = true;
      for (auto a : score_map1)
      {
         //transfer(committee_account, a.first, _core(100000));
         collect_csaf_from_committee(a.first, 100);
         account_auth_platform({ a.second }, a.first, u_9000_id, 1000 * prec, 0x1F);
         //account_manage(GRAPHENE_NULL_ACCOUNT_UID, a.first, options);
         auto b = db.get_account_by_uid(a.first);
      }
      for (auto a : score_map2)
      {
         collect_csaf_from_committee(a.first, 100);
         account_auth_platform({ a.second }, a.first, u_9000_id, 1000 * prec, 0x1F);
      }
      for (auto a : score_map3)
      {
         collect_csaf_from_committee(a.first, 100);
         account_auth_platform({ a.second }, a.first, u_9000_id, 1000 * prec, 0x1F);
      }
      for (auto a : score_map4)
      {
         collect_csaf_from_committee(a.first, 100);
         account_auth_platform({ a.second }, a.first, u_9000_id, 1000 * prec, 0x1F);
      }
      for (auto a : score_map5)
      {
         collect_csaf_from_committee(a.first, 100);
         account_auth_platform({ a.second }, a.first, u_9000_id, 1000 * prec, 0x1F);
      }



      //*************************************
      // check  victory on the side of support
      //*************************************
      for (auto a : score_map1)
         score_a_post({ a.second }, a.first, u_9000_id, u_1001_id, 1, 5, 50);
      for (auto a : score_map2)
         score_a_post({ a.second }, a.first, u_9000_id, u_1001_id, 1, -5, 10);

      generate_blocks(100);

      uint128_t award_average = (uint128_t)10000000000 * 300 / (86400 * 365);

      uint128_t post_earned = award_average;
      uint128_t score_earned = post_earned * GRAPHENE_DEFAULT_PLATFORM_RECEIPTS_RATIO / GRAPHENE_100_PERCENT;
      uint128_t receiptor_earned = post_earned - score_earned;
      uint64_t  poster_earned = (receiptor_earned * 7500 / 10000).convert_to<uint64_t>();
      auto poster_act = db.get_account_statistics_by_uid(u_1001_id);
      BOOST_CHECK(poster_act.core_balance == poster_earned);

      vector<score_id_type> scores;
      for (auto a : score_map1)
      {
         auto score_id = db.get_score(u_9000_id, u_1001_id, 1, a.first).id;
         scores.push_back(score_id);
      }
      for (auto a : score_map2)
      {
         auto score_id = db.get_score(u_9000_id, u_1001_id, 1, a.first).id;
         scores.push_back(score_id);
      }
      auto result = get_effective_csaf(scores, 50 * 20 + 10 * 20);
      share_type total_score_balance = 0;
      share_type total_reg_balance = 0;
      for (auto a : std::get<0>(result))
      {
         auto balance = score_earned.convert_to<uint64_t>() * std::get<1>(a) / std::get<1>(result);
         auto score_act = db.get_account_statistics_by_uid(std::get<0>(a));
         share_type to_reg = balance * 25 / 100;
         share_type to_score = balance - to_reg;
         total_score_balance += balance;
         total_reg_balance += to_reg;
         BOOST_CHECK(score_act.core_balance == to_score);
      }

      //check registar and referer balance
      auto reg_act = db.get_account_statistics_by_uid(genesis_state.initial_accounts.at(0).uid);
      BOOST_CHECK(reg_act.uncollected_score_bonus == total_reg_balance);

      auto platform_act = db.get_account_statistics_by_uid(u_9000_id);
      auto platform_core_balance = receiptor_earned.convert_to<uint64_t>() - poster_earned + award_average.convert_to<uint64_t>() + 10000000000;
      BOOST_CHECK(platform_act.core_balance == platform_core_balance);

      auto platform_obj = db.get_platform_by_owner(u_9000_id);
      auto post_profit = receiptor_earned.convert_to<uint64_t>() - poster_earned;
      auto iter_profit = platform_obj.period_profits.begin();
      BOOST_CHECK(iter_profit != platform_obj.period_profits.end());
      BOOST_CHECK(iter_profit->second.post_profits == post_profit);
      BOOST_CHECK(iter_profit->second.platform_profits == award_average.convert_to<uint64_t>());

      const auto& apt_idx = db.get_index_type<active_post_index>().indices().get<by_id>();
      auto active_post_obj = *(apt_idx.begin());
      BOOST_CHECK(active_post_obj.positive_win == true);
      BOOST_CHECK(active_post_obj.receiptor_details.at(u_1001_id).post_award == poster_earned);
      BOOST_CHECK(active_post_obj.post_award == (receiptor_earned.convert_to<uint64_t>() + total_score_balance));


      //*************************************
      // check  victory on the opposing side
      //*************************************
      for (auto a : score_map3)
         score_a_post({ a.second }, a.first, u_9000_id, u_1002_id, 1, 5, 50);
      for (auto a : score_map4)
         score_a_post({ a.second }, a.first, u_9000_id, u_1002_id, 1, -5, 55);

      generate_blocks(100);
      uint128_t award_average2 = (uint128_t)10000000000 * 300 / (86400 * 365);

      uint128_t post_earned2 = award_average2;
      uint128_t score_earned2 = post_earned2 * GRAPHENE_DEFAULT_PLATFORM_RECEIPTS_RATIO / GRAPHENE_100_PERCENT;
      uint128_t receiptor_earned2 = (post_earned2 - score_earned2) * 8000 / 10000;
      uint64_t  poster_earned2 = (receiptor_earned2 * 7500 / 10000).convert_to<uint64_t>();
      auto poster_act2 = db.get_account_statistics_by_uid(u_1002_id);
      BOOST_CHECK(poster_act2.core_balance == poster_earned2);

      vector<score_id_type> scores2;
      for (auto a : score_map3)
      {
         auto score_id = db.get_score(u_9000_id, u_1002_id, 1, a.first).id;
         scores2.push_back(score_id);
      }
      for (auto a : score_map4)
      {
         auto score_id = db.get_score(u_9000_id, u_1002_id, 1, a.first).id;
         scores2.push_back(score_id);
      }
      auto result2 = get_effective_csaf(scores2, 50 * 20 + 20 * 55);
      share_type total_score_balance2 = 0;
      share_type total_reg_balance2 = 0;
      for (auto a : std::get<0>(result2))
      {
         share_type balance = 0;
         if (std::get<2>(a))
            balance = (score_earned2 * std::get<1>(a).value / std::get<1>(result2).value).convert_to<uint64_t>();
         else
            balance = (score_earned2 * std::get<1>(a).value * 12000 / (std::get<1>(result2).value * 10000)).convert_to<uint64_t>();

         auto score_act = db.get_account_statistics_by_uid(std::get<0>(a));
         share_type to_reg = balance * 25 / 100;
         share_type to_score = balance - to_reg;
         total_score_balance2 += balance;
         total_reg_balance2 += to_reg;
         BOOST_CHECK(score_act.core_balance == to_score);
      }

      //check registar and referer balance
      auto reg_act2 = db.get_account_statistics_by_uid(genesis_state.initial_accounts.at(0).uid);
      BOOST_CHECK(reg_act2.uncollected_score_bonus == total_reg_balance + total_reg_balance2);

      auto platform_act2 = db.get_account_statistics_by_uid(u_9000_id);
      auto platform_core_balance2 = receiptor_earned2.convert_to<uint64_t>() - poster_earned2 + award_average2.convert_to<uint64_t>() + platform_core_balance;
      BOOST_CHECK(platform_act2.core_balance == platform_core_balance2);

      auto platform_obj2 = db.get_platform_by_owner(u_9000_id);
      auto post_profit2 = receiptor_earned2.convert_to<uint64_t>() - poster_earned2;
      auto iter_profit2 = platform_obj2.period_profits.rbegin();
      BOOST_CHECK(iter_profit2->second.post_profits == post_profit2);
      BOOST_CHECK(iter_profit2->second.platform_profits == award_average2.convert_to<uint64_t>());

      const auto& apt_idx2 = db.get_index_type<active_post_index>().indices().get<by_id>();
      auto itr = apt_idx2.begin();
      itr++;
      auto active_post_obj2 = *itr;
      BOOST_CHECK(active_post_obj2.positive_win == false);
      BOOST_CHECK(active_post_obj2.receiptor_details.at(u_1002_id).post_award == poster_earned2);
      BOOST_CHECK(active_post_obj2.post_award == (receiptor_earned2.convert_to<uint64_t>() + total_score_balance2));


      //*******************************************
      // update scorer earnings rate, check profits
      //*******************************************
      committee_update_global_extension_parameter_item_type item2;
      uint32_t scorer_earnings_rate=4000;
      item2.value.scorer_earnings_rate = scorer_earnings_rate;
      execute_proposal_head_block = db.head_block_num() + 100;
      committee_proposal_create(genesis_state.initial_accounts.at(0).uid, { item2 }, execute_proposal_head_block, voting_opinion_type::opinion_for, execute_proposal_head_block, execute_proposal_head_block);
      for (int i = 1; i < 5; ++i)
         committee_proposal_vote(genesis_state.initial_accounts.at(i).uid, 2, voting_opinion_type::opinion_for);
      generate_blocks(102);

      for (auto a : score_map5)
         score_a_post({ a.second }, a.first, u_9000_id, u_1003_id, 1, 5, 50);

      generate_blocks(100);
      uint128_t award_average3 = (uint128_t)10000000000 * 300 / (86400 * 365);

      uint128_t post_earned3 = award_average3;
      uint128_t score_earned3 = post_earned3 * scorer_earnings_rate / GRAPHENE_100_PERCENT;
      uint128_t receiptor_earned3 = post_earned3 - score_earned3;
      uint64_t  poster_earned3 = (receiptor_earned3 * 7500 / 10000).convert_to<uint64_t>();
      auto poster_act3 = db.get_account_statistics_by_uid(u_1003_id);
      BOOST_CHECK(poster_act3.core_balance == poster_earned3);

      vector<score_id_type> scores3;
      for (auto a : score_map5)
      {
         auto score_id = db.get_score(u_9000_id, u_1003_id, 1, a.first).id;
         scores3.push_back(score_id);
      }

      auto result3 = get_effective_csaf(scores3, 50 * 20);
      share_type total_score_balance3 = 0;
      share_type total_reg_balance3 = 0;
      for (auto a : std::get<0>(result3))
      {
         share_type balance = 0;
         if (std::get<2>(a))
            balance = (score_earned3 * std::get<1>(a).value / std::get<1>(result3).value).convert_to<uint64_t>();
         else
            balance = (score_earned3 * std::get<1>(a).value * 12000 / (std::get<1>(result3).value * 10000)).convert_to<uint64_t>();

         auto score_act = db.get_account_statistics_by_uid(std::get<0>(a));
         share_type to_reg = balance * 25 / 100;
         share_type to_score = balance - to_reg;
         total_score_balance3 += balance;
         total_reg_balance3 += to_reg;
         BOOST_CHECK(score_act.core_balance == to_score);
      }

      //check registar and referer balance
      auto reg_act3 = db.get_account_statistics_by_uid(genesis_state.initial_accounts.at(0).uid);
      BOOST_CHECK(reg_act3.uncollected_score_bonus == total_reg_balance + total_reg_balance2 + total_reg_balance3);

      auto platform_act3 = db.get_account_statistics_by_uid(u_9000_id);
      auto platform_core_balance3 = receiptor_earned3.convert_to<uint64_t>() - poster_earned3 + award_average3.convert_to<uint64_t>() + platform_core_balance2;
      BOOST_CHECK(platform_act3.core_balance == platform_core_balance3);

      auto platform_obj3 = db.get_platform_by_owner(u_9000_id);
      auto post_profit3 = receiptor_earned3.convert_to<uint64_t>() - poster_earned3;
      auto iter_profit3 = platform_obj3.period_profits.rbegin();
      BOOST_CHECK(iter_profit3->second.post_profits == post_profit3);
      BOOST_CHECK(iter_profit3->second.platform_profits == award_average3.convert_to<uint64_t>());

      const auto& apt_idx3 = db.get_index_type<active_post_index>().indices().get<by_id>();
      auto itr3 = apt_idx3.rbegin();
      auto active_post_obj3 = *itr3;
      BOOST_CHECK(active_post_obj3.positive_win == true);
      BOOST_CHECK(active_post_obj3.receiptor_details.at(u_1003_id).post_award == poster_earned3);
      BOOST_CHECK(active_post_obj3.post_award == (receiptor_earned3.convert_to<uint64_t>() + total_score_balance3));

   }
   catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}


//test api: process_platform_voted_awards()
BOOST_AUTO_TEST_CASE(platform_voted_awards_test)
{
   try{
      //ACTORS((1001)(9000));

      flat_map<account_uid_type, fc::ecc::private_key> platform_map1;
      flat_map<account_uid_type, fc::ecc::private_key> platform_map2;
      actor(8001, 5, platform_map1);
      actor(9001, 5, platform_map2);
      flat_set<account_uid_type> platform_set1;
      flat_set<account_uid_type> platform_set2;
      for (const auto&p : platform_map1)
         platform_set1.insert(p.first);
      for (const auto&p : platform_map2)
         platform_set2.insert(p.first);

      flat_map<account_uid_type, fc::ecc::private_key> vote_map1;
      flat_map<account_uid_type, fc::ecc::private_key> vote_map2;
      actor(1003, 10, vote_map1);
      actor(2003, 20, vote_map2);

      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);

      generate_blocks(200000);

      // Return number of core shares (times precision)
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };


      for (int i = 0; i < 5; ++i)
         add_csaf_for_account(genesis_state.initial_accounts.at(i).uid, 1000);

      for (const auto &p : platform_map1)
      {
         transfer(committee_account, p.first, _core(100000));
         add_csaf_for_account(p.first, 1000);
      }
      for (const auto &p : platform_map2)
      {
         transfer(committee_account, p.first, _core(100000));
         add_csaf_for_account(p.first, 1000);
      }
      for (const auto &v : vote_map1)
      {
         transfer(committee_account, v.first, _core(10000));
         add_csaf_for_account(v.first, 1000);
      }
      for (const auto &v : vote_map2)
      {
         transfer(committee_account, v.first, _core(10000));
         add_csaf_for_account(v.first, 1000);
      }

      uint32_t i = 0;
      for (const auto &p : platform_map1)
      {
         create_platform(p.first, "platform" + i, _core(10000), "www.123456789.com" + i, "", { p.second });
         i++;
      }
      for (const auto &p : platform_map2)
      {
         create_platform(p.first, "platform" + i, _core(10000), "www.123456789.com" + i, "", { p.second });
         i++;
      }

      uint32_t current_block_num = db.head_block_num();

      BOOST_TEST_MESSAGE("Turn on the reward mechanism, open content award and platform voted award");
      committee_update_global_extension_parameter_item_type content_item;
      content_item.value = { 300, 300, 1000, 31536000, 10, 0, 0, 10000000000, 100, 10 };
      committee_proposal_create(genesis_state.initial_accounts.at(0).uid, { content_item }, current_block_num + 10, voting_opinion_type::opinion_for, current_block_num + 10, current_block_num + 10);
      committee_update_global_parameter_item_type item;
      item.value.governance_votes_update_interval = 20;
      committee_proposal_create(genesis_state.initial_accounts.at(0).uid, { item }, current_block_num + 10, voting_opinion_type::opinion_for, current_block_num + 10, current_block_num + 10);
      for (int i = 1; i < 5; ++i)
      {
         committee_proposal_vote(genesis_state.initial_accounts.at(i).uid, 1, voting_opinion_type::opinion_for);
         committee_proposal_vote(genesis_state.initial_accounts.at(i).uid, 2, voting_opinion_type::opinion_for);
      }

      generate_blocks(10);

      flat_set<account_uid_type> empty;
      for (const auto &v : vote_map1)
      {
         update_platform_votes(v.first, platform_set1, empty, { v.second });
      }
      for (const auto &v : vote_map2)
      {
         update_platform_votes(v.first, platform_set2, empty, { v.second });
      }

      generate_blocks(100);

      uint128_t award = (uint128_t)10000000000 * 300 / (86400 * 365);
      uint128_t platform_award_basic = award * 2000 / 10000;
      uint128_t basic = platform_award_basic / (platform_map1.size() + platform_map2.size());
      uint128_t platform_award_by_votes = award - platform_award_basic;

      uint32_t total_vote = 46293 * (10 + 20) * 5;
      for (const auto&p : platform_map1)
      {
         uint32_t votes = 46293 * 10;
         uint128_t award_by_votes = platform_award_by_votes * votes / total_vote;
         share_type balance = (award_by_votes + basic).convert_to<uint64_t>();
         auto pla_act = db.get_account_statistics_by_uid(p.first);
         BOOST_CHECK(pla_act.core_balance == balance + 10000000000);
         auto platform_obj = db.get_platform_by_owner(p.first);
         BOOST_CHECK(platform_obj.vote_profits.begin()->second == balance);
      }
      for (const auto&p : platform_map2)
      {
         uint32_t votes = 46293 * 20;
         uint128_t award_by_votes = platform_award_by_votes * votes / total_vote;
         share_type balance = (award_by_votes + basic).convert_to<uint64_t>();
         auto pla_act = db.get_account_statistics_by_uid(p.first);
         BOOST_CHECK(pla_act.core_balance == balance + 10000000000);
         auto platform_obj = db.get_platform_by_owner(p.first);
         BOOST_CHECK(platform_obj.vote_profits.begin()->second == balance);
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
      const _account_statistics_object& temp = db.get_account_statistics_by_uid(u_1000_id);

      // make sure the database requires our fee to be nonzero
      enable_fees();

      flat_set<fc::ecc::private_key> sign_keys;
      sign_keys.insert(u_1000_private_key);
      transfer_extension(sign_keys, u_1000_id, u_1000_id, _core(6000), "", true, false);
      const _account_statistics_object& ant1000 = db.get_account_statistics_by_uid(u_1000_id);
      BOOST_CHECK(ant1000.prepaid == 6000 * prec);
      BOOST_CHECK(ant1000.core_balance == 4000 * prec);

      transfer_extension(sign_keys, u_1000_id, u_1001_id, _core(5000), "", false, true);
      const _account_statistics_object& ant1000_1 = db.get_account_statistics_by_uid(u_1000_id);
      const _account_statistics_object& ant1001 = db.get_account_statistics_by_uid(u_1001_id);
      BOOST_CHECK(ant1000_1.prepaid == 1000 * prec);
      BOOST_CHECK(ant1001.core_balance == 15000 * prec);

      flat_set<fc::ecc::private_key> sign_keys1;
      sign_keys1.insert(u_1001_private_key);
      transfer_extension(sign_keys1, u_1001_id, u_1000_id, _core(15000), "", true, true);
      const _account_statistics_object& ant1000_2 = db.get_account_statistics_by_uid(u_1000_id);
      BOOST_CHECK(ant1000_2.prepaid == 1000 * prec);
      BOOST_CHECK(ant1000_2.core_balance == 19000 * prec);

      transfer_extension(sign_keys, u_1000_id, u_1001_id, _core(1000), "", false, false);
      const _account_statistics_object& ant1001_2 = db.get_account_statistics_by_uid(u_1001_id);
      const _account_statistics_object& ant1000_3 = db.get_account_statistics_by_uid(u_1000_id);
      BOOST_CHECK(ant1001_2.prepaid == 1000 * prec);
      BOOST_CHECK(ant1000_3.prepaid == 0);

      generate_blocks(HARDFORK_0_4_TIME, true);
      account_auth_platform({ u_2000_private_key }, u_2000_id, u_9000_id, 1000 * prec, account_auth_platform_object::Platform_Permission_Forward |
         account_auth_platform_object::Platform_Permission_Liked |
         account_auth_platform_object::Platform_Permission_Buyout |
         account_auth_platform_object::Platform_Permission_Comment |
         account_auth_platform_object::Platform_Permission_Reward |
         account_auth_platform_object::Platform_Permission_Transfer |
         account_auth_platform_object::Platform_Permission_Post |
         account_auth_platform_object::Platform_Permission_Content_Update);
      transfer_extension({ u_2000_private_key }, u_2000_id, u_2000_id, _core(10000), "", true, false);
      transfer_extension({ u_9000_private_key }, u_2000_id, u_9000_id, _core(1000), "", false, true);
      const _account_statistics_object& ant2000 = db.get_account_statistics_by_uid(u_2000_id);
      const _account_statistics_object& ant9000 = db.get_account_statistics_by_uid(u_9000_id);
      BOOST_CHECK(ant2000.prepaid == 9000 * prec);
      BOOST_CHECK(ant9000.core_balance == 1000 * prec);

   }
   catch (fc::exception& e) {
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
      generate_blocks(HARDFORK_0_4_TIME, true);

      flat_set<fc::ecc::private_key> sign_keys1;
      sign_keys1.insert(u_1000_private_key);
      account_auth_platform(sign_keys1, u_1000_id, u_9000_id, 1000 * prec, account_auth_platform_object::Platform_Permission_Forward |
         account_auth_platform_object::Platform_Permission_Liked |
         account_auth_platform_object::Platform_Permission_Buyout |
         account_auth_platform_object::Platform_Permission_Comment |
         account_auth_platform_object::Platform_Permission_Reward |
         account_auth_platform_object::Platform_Permission_Transfer |
         account_auth_platform_object::Platform_Permission_Post |
         account_auth_platform_object::Platform_Permission_Content_Update);

      const account_auth_platform_object& ant1000 = db.get_account_auth_platform_object_by_account_platform(u_1000_id, u_9000_id);
      BOOST_CHECK(ant1000.max_limit == 1000 * prec);
      BOOST_CHECK(ant1000.permission_flags & account_auth_platform_object::Platform_Permission_Forward);
      BOOST_CHECK(ant1000.permission_flags & account_auth_platform_object::Platform_Permission_Liked);
      BOOST_CHECK(ant1000.permission_flags & account_auth_platform_object::Platform_Permission_Buyout);
      BOOST_CHECK(ant1000.permission_flags & account_auth_platform_object::Platform_Permission_Comment);
      BOOST_CHECK(ant1000.permission_flags & account_auth_platform_object::Platform_Permission_Reward);
      BOOST_CHECK(ant1000.permission_flags & account_auth_platform_object::Platform_Permission_Transfer);
      BOOST_CHECK(ant1000.permission_flags & account_auth_platform_object::Platform_Permission_Post);
      BOOST_CHECK(ant1000.permission_flags & account_auth_platform_object::Platform_Permission_Content_Update);


      account_auth_platform(sign_keys1, u_1000_id, u_9000_id, 6000 * prec, 0);

      const account_auth_platform_object& ant10001 = db.get_account_auth_platform_object_by_account_platform(u_1000_id, u_9000_id);
      BOOST_CHECK(ant10001.max_limit == 6000 * prec);
      BOOST_CHECK((ant10001.permission_flags & account_auth_platform_object::Platform_Permission_Forward) == 0);
      BOOST_CHECK((ant10001.permission_flags & account_auth_platform_object::Platform_Permission_Liked) == 0);
      BOOST_CHECK((ant10001.permission_flags & account_auth_platform_object::Platform_Permission_Buyout) == 0);
      BOOST_CHECK((ant10001.permission_flags & account_auth_platform_object::Platform_Permission_Comment) == 0);
      BOOST_CHECK((ant10001.permission_flags & account_auth_platform_object::Platform_Permission_Reward) == 0);
      BOOST_CHECK((ant10001.permission_flags & account_auth_platform_object::Platform_Permission_Transfer) == 0);
      BOOST_CHECK((ant10001.permission_flags & account_auth_platform_object::Platform_Permission_Post) == 0);
      BOOST_CHECK((ant10001.permission_flags & account_auth_platform_object::Platform_Permission_Content_Update) == 0);
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
      generate_blocks(HARDFORK_0_4_TIME, true);

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
      generate_blocks(HARDFORK_0_4_TIME, true);
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
      sign_keys1.insert(u_9000_private_key);

      map<account_uid_type, Receiptor_Parameter> receiptors;
      receiptors.insert(std::make_pair(u_9000_id, Receiptor_Parameter{ GRAPHENE_DEFAULT_PLATFORM_RECEIPTS_RATIO, false, 0, 0 }));
      receiptors.insert(std::make_pair(u_1000_id, Receiptor_Parameter{ 5000, false, 0, 0 }));
      receiptors.insert(std::make_pair(u_2000_id, Receiptor_Parameter{ 2500, false, 0, 0 }));

      post_operation::ext extension;
      extension.post_type = post_operation::Post_Type_Post;
      extension.forward_price = 10000 * prec;
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
      Receiptor_Parameter r9 = post.receiptors.find(u_9000_id)->second;
      BOOST_CHECK(r9 == Receiptor_Parameter(GRAPHENE_DEFAULT_PLATFORM_RECEIPTS_RATIO, false, 0, 0));
      BOOST_CHECK(post.receiptors.find(u_1000_id) != post.receiptors.end());
      Receiptor_Parameter r1 = post.receiptors.find(u_1000_id)->second;
      BOOST_CHECK(r1 == Receiptor_Parameter(5000, false, 0, 0));
      BOOST_CHECK(post.receiptors.find(u_2000_id) != post.receiptors.end());
      Receiptor_Parameter r2 = post.receiptors.find(u_2000_id)->second;
      BOOST_CHECK(r2 == Receiptor_Parameter(2500, false, 0, 0));

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
      generate_blocks(HARDFORK_0_4_TIME, true);
      create_license(u_9000_id, 6, "999999999", "license title", "license body", "extra", sign_keys);

      flat_set<fc::ecc::private_key> sign_keys1;
      flat_set<fc::ecc::private_key> sign_keys2;
      sign_keys1.insert(u_1000_private_key);
      sign_keys2.insert(u_2000_private_key);
      account_auth_platform(sign_keys1, u_1000_id, u_9000_id, 1000 * prec, account_auth_platform_object::Platform_Permission_Forward |
         account_auth_platform_object::Platform_Permission_Liked |
         account_auth_platform_object::Platform_Permission_Buyout |
         account_auth_platform_object::Platform_Permission_Comment |
         account_auth_platform_object::Platform_Permission_Reward |
         account_auth_platform_object::Platform_Permission_Post |
         account_auth_platform_object::Platform_Permission_Content_Update);
      account_auth_platform(sign_keys2, u_2000_id, u_9000_id, 1000 * prec, account_auth_platform_object::Platform_Permission_Forward |
         account_auth_platform_object::Platform_Permission_Liked |
         account_auth_platform_object::Platform_Permission_Buyout |
         account_auth_platform_object::Platform_Permission_Comment |
         account_auth_platform_object::Platform_Permission_Reward |
         account_auth_platform_object::Platform_Permission_Post |
         account_auth_platform_object::Platform_Permission_Content_Update);
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
      BOOST_CHECK(comment.origin_platform == u_9000_id);
      BOOST_CHECK(comment.origin_poster == u_1000_id);
      BOOST_CHECK(comment.origin_post_pid == 1);
      BOOST_CHECK(comment.hash_value == "2333333");
      BOOST_CHECK(comment.title == "comment");
      BOOST_CHECK(comment.body == "the post is good");
      BOOST_CHECK(comment.extra_data == "extra");
      BOOST_CHECK(comment.forward_price == 10000 * prec);
      BOOST_CHECK(comment.license_lid == 1);
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

      generate_blocks(10);

      BOOST_TEST_MESSAGE("Turn on the reward mechanism, open content award and platform voted award");
      committee_update_global_extension_parameter_item_type item;
      item.value = { 300, 300, 1000, 31536000, 10, 10000000000000, 10000000000000, 10000000000000, 1000, 100 };
      committee_proposal_create(genesis_state.initial_accounts.at(0).uid, { item }, 100, voting_opinion_type::opinion_for, 100, 100);
      for (int i = 1; i < 5; ++i)
         committee_proposal_vote(genesis_state.initial_accounts.at(i).uid, 1, voting_opinion_type::opinion_for);
      generate_blocks(89);

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

      generate_blocks(HARDFORK_0_4_TIME, true);
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
      account_auth_platform(sign_keys_1, u_1000_id, u_9000_id, 10000 * prec, account_auth_platform_object::Platform_Permission_Forward |
         account_auth_platform_object::Platform_Permission_Liked |
         account_auth_platform_object::Platform_Permission_Buyout |
         account_auth_platform_object::Platform_Permission_Comment |
         account_auth_platform_object::Platform_Permission_Reward |
         account_auth_platform_object::Platform_Permission_Post |
         account_auth_platform_object::Platform_Permission_Content_Update);
      account_auth_platform(sign_keys_2, u_2000_id, u_9001_id, 10000 * prec, account_auth_platform_object::Platform_Permission_Forward |
         account_auth_platform_object::Platform_Permission_Liked |
         account_auth_platform_object::Platform_Permission_Buyout |
         account_auth_platform_object::Platform_Permission_Comment |
         account_auth_platform_object::Platform_Permission_Reward |
         account_auth_platform_object::Platform_Permission_Post |
         account_auth_platform_object::Platform_Permission_Content_Update);
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
      if (do_by_platform){
         extension.sign_platform = u_9001_id;
      }
      create_post(sign_keys_2, u_9001_id, u_2000_id, "9999999", "new titile", "new body", "extra", u_9000_id, u_1000_id, 1, extension);

      const post_object& forward_post = db.get_post_by_platform(u_9001_id, u_2000_id, 1);
      BOOST_CHECK(forward_post.origin_platform == u_9000_id);
      BOOST_CHECK(forward_post.origin_poster == u_1000_id);
      BOOST_CHECK(forward_post.origin_post_pid == 1);
      BOOST_CHECK(forward_post.hash_value == "9999999");
      BOOST_CHECK(forward_post.title == "new titile");
      BOOST_CHECK(forward_post.body == "new body");
      BOOST_CHECK(forward_post.extra_data == "extra");
      BOOST_CHECK(forward_post.forward_price == 10000 * prec);
      BOOST_CHECK(forward_post.license_lid == 1);
      BOOST_CHECK(forward_post.permission_flags == post_object::Post_Permission_Forward |
         post_object::Post_Permission_Liked |
         post_object::Post_Permission_Buyout |
         post_object::Post_Permission_Comment |
         post_object::Post_Permission_Reward);

      const _account_statistics_object& sobj1 = db.get_account_statistics_by_uid(u_1000_id);
      BOOST_CHECK(sobj1.prepaid == 17500 * prec);
      const _account_statistics_object& platform1 = db.get_account_statistics_by_uid(u_9000_id);
      BOOST_CHECK(platform1.prepaid == 2500 * prec);
      BOOST_CHECK(platform1.core_balance == 10000 * prec);
      const _account_statistics_object& sobj2 = db.get_account_statistics_by_uid(u_2000_id);
      BOOST_CHECK(sobj2.prepaid == 0);

      if (do_by_platform){
         //auto auth_data = sobj2.prepaids_for_platform.find(u_9001_id);
         //BOOST_CHECK(auth_data != sobj2.prepaids_for_platform.end());
         //BOOST_CHECK(auth_data->second.cur_used == 10000*prec);
         //BOOST_CHECK(sobj2.get_auth_platform_usable_prepaid(u_9001_id) == 0);

         auto auth_data = db.get_account_auth_platform_object_by_account_platform(u_2000_id, u_9001_id);
         BOOST_CHECK(auth_data.cur_used == 10000 * prec);
         BOOST_CHECK(auth_data.get_auth_platform_usable_prepaid(sobj2.prepaid) == 0);
      }

      auto dpo = db.get_dynamic_global_properties();
      const auto& apt_idx = db.get_index_type<active_post_index>().indices().get<by_post_pid>();
      auto apt_itr = apt_idx.find(std::make_tuple(u_9000_id, u_1000_id, dpo.current_active_post_sequence, 1));
      BOOST_CHECK(apt_itr != apt_idx.end());
      auto active_post = *apt_itr;
      BOOST_CHECK(active_post.forward_award == 10000 * prec);
      auto iter_receiptor = active_post.receiptor_details.find(u_1000_id);
      BOOST_CHECK(iter_receiptor != active_post.receiptor_details.end());
      BOOST_CHECK(iter_receiptor->second.forward == 7500 * prec);
      auto iter_receiptor2 = active_post.receiptor_details.find(u_9000_id);
      BOOST_CHECK(iter_receiptor2 != active_post.receiptor_details.end());
      BOOST_CHECK(iter_receiptor2->second.forward == 2500 * prec);

      const platform_object& platform = db.get_platform_by_owner(u_9000_id);
      auto iter_profit = platform.period_profits.find(dpo.current_active_post_sequence);
      BOOST_CHECK(iter_profit != platform.period_profits.end());
      BOOST_CHECK(iter_profit->second.foward_profits == 2500 * prec);
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

      generate_blocks(HARDFORK_0_4_TIME, true);
      flat_set<fc::ecc::private_key> sign_keys;
      sign_keys.insert(u_9000_private_key);
      create_platform(u_9000_id, "platform", _core(10000), "www.123456789.com", "", sign_keys);
      create_license(u_9000_id, 6, "999999999", "license title", "license body", "extra", sign_keys);


      flat_set<fc::ecc::private_key> sign_keys_1;
      flat_set<fc::ecc::private_key> sign_keys_2;
      sign_keys_1.insert(u_1000_private_key);
      sign_keys_2.insert(u_2000_private_key);
      account_auth_platform(sign_keys_1, u_1000_id, u_9000_id, 10000 * prec, account_auth_platform_object::Platform_Permission_Forward |
         account_auth_platform_object::Platform_Permission_Liked |
         account_auth_platform_object::Platform_Permission_Buyout |
         account_auth_platform_object::Platform_Permission_Comment |
         account_auth_platform_object::Platform_Permission_Reward |
         account_auth_platform_object::Platform_Permission_Post |
         account_auth_platform_object::Platform_Permission_Content_Update);
      account_auth_platform(sign_keys_2, u_2000_id, u_9000_id, 10000 * prec, account_auth_platform_object::Platform_Permission_Forward |
         account_auth_platform_object::Platform_Permission_Liked |
         account_auth_platform_object::Platform_Permission_Buyout |
         account_auth_platform_object::Platform_Permission_Comment |
         account_auth_platform_object::Platform_Permission_Reward |
         account_auth_platform_object::Platform_Permission_Post |
         account_auth_platform_object::Platform_Permission_Content_Update);
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

      buyout_post(u_2000_id, u_9000_id, u_1000_id, 1, u_1000_id, u_9000_id, sign_keys_2);

      const post_object& post = db.get_post_by_platform(u_9000_id, u_1000_id, 1);
      auto iter1 = post.receiptors.find(u_1000_id);
      BOOST_CHECK(iter1 != post.receiptors.end());
      BOOST_CHECK(iter1->second.cur_ratio == 4500);
      BOOST_CHECK(iter1->second.to_buyout == false);
      BOOST_CHECK(iter1->second.buyout_ratio == 0);
      BOOST_CHECK(iter1->second.buyout_price == 0);

      auto iter2 = post.receiptors.find(u_2000_id);
      BOOST_CHECK(iter2 != post.receiptors.end());
      BOOST_CHECK(iter2->second.cur_ratio == 3000);
      BOOST_CHECK(iter2->second.to_buyout == false);
      BOOST_CHECK(iter2->second.buyout_ratio == 0);
      BOOST_CHECK(iter2->second.buyout_price == 0);

      const _account_statistics_object& sobj1 = db.get_account_statistics_by_uid(u_1000_id);
      BOOST_CHECK(sobj1.prepaid == 11000 * prec);
      const _account_statistics_object& sobj2 = db.get_account_statistics_by_uid(u_2000_id);
      BOOST_CHECK(sobj2.prepaid == 9000 * prec);

      if (do_by_platform){
         //auto auth_data = sobj2.prepaids_for_platform.find(u_9000_id);
         //BOOST_CHECK(auth_data != sobj2.prepaids_for_platform.end());
         //BOOST_CHECK(auth_data->second.cur_used == 1000 * prec);

         auto auth_data = db.get_account_auth_platform_object_by_account_platform(u_2000_id, u_9000_id);
         BOOST_CHECK(auth_data.cur_used == 1000 * prec);
      }
   }
   catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(advertising_test)
{
   try{
      ACTORS((1000)(2000)(3000)(4000)(9000));

      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };
      transfer(committee_account, u_1000_id, _core(10000));
      transfer(committee_account, u_2000_id, _core(10000));
      transfer(committee_account, u_3000_id, _core(10000));
      transfer(committee_account, u_4000_id, _core(10000));
      transfer(committee_account, u_9000_id, _core(10000));
      add_csaf_for_account(u_1000_id, 10000);
      add_csaf_for_account(u_2000_id, 10000);
      add_csaf_for_account(u_3000_id, 10000);
      add_csaf_for_account(u_4000_id, 10000);
      add_csaf_for_account(u_9000_id, 10000);
      generate_blocks(HARDFORK_0_4_TIME, true);

      create_platform(u_9000_id, "platform", _core(10000), "www.123456789.com", "", { u_9000_private_key });
      create_advertising({ u_9000_private_key }, u_9000_id, "this is a test", share_type(100000000), 100000);
      generate_blocks(10);
      uint32_t unit_time = 100000;
      const auto& idx = db.get_index_type<advertising_index>().indices().get<by_advertising_platform>();
      const auto& obj = *(idx.begin());
      BOOST_CHECK(obj.description == "this is a test");
      BOOST_CHECK(obj.unit_time == unit_time);
      BOOST_CHECK(obj.unit_price.value == 100000000);

      time_point_sec now_time = db.head_block_time();
      time_point_sec time1 = now_time + 300;
      time_point_sec time2 = time1 + 2 * unit_time + 300;
      buy_advertising({ u_1000_private_key }, u_1000_id, u_9000_id, 1, time1, 2, "u_1000", "");
      buy_advertising({ u_2000_private_key }, u_2000_id, u_9000_id, 1, time1, 2, "u_2000", "");
      buy_advertising({ u_3000_private_key }, u_3000_id, u_9000_id, 1, time1, 2, "u_3000", "");
      buy_advertising({ u_4000_private_key }, u_4000_id, u_9000_id, 1, time2, 2, "u_4000", "");

      const auto& idx_order = db.get_index_type<advertising_order_index>().indices().get<by_advertising_user_id>();
      auto itr1 = idx_order.lower_bound(u_1000_id);
      BOOST_CHECK(itr1 != idx_order.end());
      BOOST_CHECK(itr1->user == u_1000_id);
      BOOST_CHECK(itr1->released_balance == 100000000 * 2);
      BOOST_CHECK(itr1->start_time == time1);

      auto itr2 = idx_order.lower_bound(u_2000_id);
      BOOST_CHECK(itr2 != idx_order.end());
      BOOST_CHECK(itr2->user == u_2000_id);
      BOOST_CHECK(itr2->released_balance == 100000000 * 2);
      BOOST_CHECK(itr2->start_time == time1);

      auto itr3 = idx_order.lower_bound(u_3000_id);
      BOOST_CHECK(itr3 != idx_order.end());
      BOOST_CHECK(itr3->user == u_3000_id);
      BOOST_CHECK(itr3->released_balance == 100000000 * 2);
      BOOST_CHECK(itr3->start_time == time1);

      auto itr4 = idx_order.lower_bound(u_4000_id);
      BOOST_CHECK(itr4 != idx_order.end());
      BOOST_CHECK(itr4->user == u_4000_id);
      BOOST_CHECK(itr4->released_balance == 100000000 * 2);
      BOOST_CHECK(itr4->start_time == time2);

      const auto& user1 = db.get_account_statistics_by_uid(u_1000_id);
      BOOST_CHECK(user1.core_balance == 8000 * prec);
      const auto& user2 = db.get_account_statistics_by_uid(u_2000_id);
      BOOST_CHECK(user2.core_balance == 8000 * prec);
      const auto& user3 = db.get_account_statistics_by_uid(u_3000_id);
      BOOST_CHECK(user3.core_balance == 8000 * prec);
      const auto& user4 = db.get_account_statistics_by_uid(u_4000_id);
      BOOST_CHECK(user4.core_balance == 8000 * prec);

      confirm_advertising({ u_9000_private_key }, u_9000_id, obj.advertising_aid, 1, true);

      const auto& idx_ordered = db.get_index_type<advertising_order_index>().indices().get<by_advertising_order_state>();
      auto itr6 = idx_ordered.lower_bound(advertising_accepted);
      const advertising_order_object adobj1 = *itr6;
      BOOST_CHECK(itr6 != idx_ordered.end());
      BOOST_CHECK(itr6->user == u_1000_id);
      BOOST_CHECK(itr6->released_balance == 2000 * prec);
      BOOST_CHECK(itr6->start_time == time1);

      confirm_advertising({ u_9000_private_key }, u_9000_id, obj.advertising_aid, 4, false);

      BOOST_CHECK(user1.core_balance == 8000 * prec);
      BOOST_CHECK(user2.core_balance == 10000 * prec);
      BOOST_CHECK(user3.core_balance == 10000 * prec);
      BOOST_CHECK(user4.core_balance == 10000 * prec);

      const auto& platform = db.get_account_statistics_by_uid(u_9000_id);
      BOOST_CHECK(platform.core_balance == (12000 - 20) * prec);

      update_advertising({ u_9000_private_key }, u_9000_id, obj.advertising_aid, "this is advertising test", share_type(200000000), 100000, optional<bool>());


      BOOST_CHECK(obj.description == "this is advertising test");
      BOOST_CHECK(obj.unit_time == unit_time);
      BOOST_CHECK(obj.unit_price.value == 200000000);

      const auto& idx_by_clear_time = db.get_index_type<advertising_order_index>().indices().get<by_clear_time>();
      //auto itr_by_clear_time = 
   }
   catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(custom_vote_test)
{
   try{
      ACTORS((1000)(2000)(3000)(4000)(9000));

      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };
      transfer(committee_account, u_1000_id, _core(10000));
      transfer(committee_account, u_2000_id, _core(10000));
      transfer(committee_account, u_3000_id, _core(10000));
      transfer(committee_account, u_4000_id, _core(10000));
      transfer(committee_account, u_9000_id, _core(10000));
      add_csaf_for_account(u_1000_id, 10000);
      add_csaf_for_account(u_2000_id, 10000);
      add_csaf_for_account(u_3000_id, 10000);
      add_csaf_for_account(u_4000_id, 10000);
      add_csaf_for_account(u_9000_id, 10000);
      generate_blocks(HARDFORK_0_4_TIME, true);


      create_custom_vote({ u_9000_private_key }, u_9000_id, 1, "title", "description", db.head_block_time() + 100000,
         0, share_type(1000000), 1, 3, { "aa", "bb", "cc", "dd" });


      //***************************************************
      //must start non_consensus custom vote, or check error
      //***************************************************
      const auto& idx = db.get_index_type<custom_vote_index>().indices().get<by_id>();
      const auto& obj = *(idx.begin());
      BOOST_CHECK(obj.custom_vote_creator == u_9000_id);
      BOOST_CHECK(obj.vote_vid == 1);
      BOOST_CHECK(obj.title == "title");
      BOOST_CHECK(obj.description == "description");
      BOOST_CHECK(obj.vote_expired_time == db.head_block_time() + 100000);
      BOOST_CHECK(obj.required_asset_amount.value == 1000000);
      BOOST_CHECK(obj.vote_asset_id == 0);
      BOOST_CHECK(obj.minimum_selected_items == 1);
      BOOST_CHECK(obj.maximum_selected_items == 3);
      BOOST_CHECK(obj.options.size() == 4);
      BOOST_CHECK(obj.options.at(0) == "aa");
      BOOST_CHECK(obj.options.at(1) == "bb");
      BOOST_CHECK(obj.options.at(2) == "cc");
      BOOST_CHECK(obj.options.at(3) == "dd");

      cast_custom_vote({ u_1000_private_key }, u_1000_id, u_9000_id, 1, { 0, 1 });
      BOOST_CHECK(obj.vote_result.at(0) == 10000 * prec);
      BOOST_CHECK(obj.vote_result.at(1) == 10000 * prec);
      BOOST_CHECK(obj.vote_result.at(2) == 0);
      BOOST_CHECK(obj.vote_result.at(3) == 0);

      cast_custom_vote({ u_2000_private_key }, u_2000_id, u_9000_id, 1, { 0, 1, 2 });
      BOOST_CHECK(obj.vote_result.at(0) == 20000 * prec);
      BOOST_CHECK(obj.vote_result.at(1) == 20000 * prec);
      BOOST_CHECK(obj.vote_result.at(2) == 10000 * prec);
      BOOST_CHECK(obj.vote_result.at(3) == 0);

      cast_custom_vote({ u_3000_private_key }, u_3000_id, u_9000_id, 1, { 2, 3 });
      BOOST_CHECK(obj.vote_result.at(0) == 20000 * prec);
      BOOST_CHECK(obj.vote_result.at(1) == 20000 * prec);
      BOOST_CHECK(obj.vote_result.at(2) == 20000 * prec);
      BOOST_CHECK(obj.vote_result.at(3) == 10000 * prec);

      cast_custom_vote({ u_4000_private_key }, u_4000_id, u_9000_id, 1, { 1, 3 });
      BOOST_CHECK(obj.vote_result.at(0) == 20000 * prec);
      BOOST_CHECK(obj.vote_result.at(1) == 30000 * prec);
      BOOST_CHECK(obj.vote_result.at(2) == 20000 * prec);
      BOOST_CHECK(obj.vote_result.at(3) == 20000 * prec);

      transfer(committee_account, u_1000_id, _core(40000));
      BOOST_CHECK(obj.vote_result.at(0) == 60000 * prec);
      BOOST_CHECK(obj.vote_result.at(1) == 70000 * prec);
      BOOST_CHECK(obj.vote_result.at(2) == 20000 * prec);
      BOOST_CHECK(obj.vote_result.at(3) == 20000 * prec);

      transfer(u_3000_id, u_1000_id, _core(5000));
      BOOST_CHECK(obj.vote_result.at(0) == 65000 * prec);
      BOOST_CHECK(obj.vote_result.at(1) == 75000 * prec);
      BOOST_CHECK(obj.vote_result.at(2) == 15000 * prec);
      BOOST_CHECK(obj.vote_result.at(3) == 15000 * prec);

   }
   catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(balance_lock_for_feepoint_test)
{
   try{
      ACTORS((1000)(2000));

      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };
      transfer(committee_account, u_1000_id, _core(10000));
      transfer(committee_account, u_2000_id, _core(10000));
      add_csaf_for_account(u_1000_id, 10000);
      add_csaf_for_account(u_2000_id, 10000);
      generate_blocks(HARDFORK_0_5_TIME, true);


      balance_lock_update({ u_1000_private_key }, u_1000_id, 5000 * prec);
      balance_lock_update({ u_2000_private_key }, u_2000_id, 8000 * prec);

      //test update pledge
      const auto& user1 = db.get_account_statistics_by_uid(u_1000_id);
      const auto& pledge_balance_obj1 = user1.pledge_balance_ids.at(pledge_balance_type::Lock_balance)(db);
      BOOST_CHECK(pledge_balance_obj1.pledge == 5000 * prec);
      BOOST_CHECK(pledge_balance_obj1.total_releasing_pledge == 0 * prec);
      BOOST_CHECK(pledge_balance_obj1.releasing_pledges.size() == 0);
      const auto& user2 = db.get_account_statistics_by_uid(u_2000_id);
      const auto& pledge_balance_obj2 = user2.pledge_balance_ids.at(pledge_balance_type::Lock_balance)(db);
      BOOST_CHECK(pledge_balance_obj2.pledge == 8000 * prec);
      BOOST_CHECK(pledge_balance_obj2.total_releasing_pledge == 0 * prec);
      BOOST_CHECK(pledge_balance_obj2.releasing_pledges.size() == 0 * prec);

      //test new releasing
      balance_lock_update({ u_1000_private_key }, u_1000_id, 6000 * prec);
      balance_lock_update({ u_2000_private_key }, u_2000_id, 5000 * prec);
      const auto& user3 = db.get_account_statistics_by_uid(u_1000_id);
      const auto& pledge_balance_obj3 = user3.pledge_balance_ids.at(pledge_balance_type::Lock_balance)(db);
      BOOST_CHECK(pledge_balance_obj3.pledge == 6000 * prec);
      BOOST_CHECK(pledge_balance_obj3.total_releasing_pledge == 0 * prec);
      BOOST_CHECK(pledge_balance_obj3.releasing_pledges.size() == 0 );
      const auto& user4 = db.get_account_statistics_by_uid(u_2000_id);
      const auto& pledge_balance_obj4 = user4.pledge_balance_ids.at(pledge_balance_type::Lock_balance)(db);
      BOOST_CHECK(pledge_balance_obj4.pledge == 5000 * prec);
      BOOST_CHECK(pledge_balance_obj4.total_releasing_pledge == 3000 * prec);
      BOOST_CHECK(pledge_balance_obj4.releasing_pledges.size() == 1);
      BOOST_CHECK(pledge_balance_obj4.releasing_pledges.begin()->first == db.head_block_num() + GRAPHENE_DEFAULT_UNLOCKED_BALANCE_RELEASE_DELAY);
      BOOST_CHECK(pledge_balance_obj4.releasing_pledges.begin()->second == 3000 * prec);

      generate_blocks(10);
      //test new releasing
      balance_lock_update({ u_2000_private_key }, u_2000_id, 3000 * prec);
      const auto& user5 = db.get_account_statistics_by_uid(u_2000_id);
      const auto& pledge_balance_obj5 = user5.pledge_balance_ids.at(pledge_balance_type::Lock_balance)(db);
      BOOST_CHECK(pledge_balance_obj5.pledge == 3000 * prec);
      BOOST_CHECK(pledge_balance_obj5.total_releasing_pledge == 5000 * prec);
      BOOST_CHECK(pledge_balance_obj5.releasing_pledges.size() == 2);
      BOOST_CHECK(pledge_balance_obj5.releasing_pledges.rbegin()->first == db.head_block_num() + GRAPHENE_DEFAULT_UNLOCKED_BALANCE_RELEASE_DELAY);
      BOOST_CHECK(pledge_balance_obj5.releasing_pledges.rbegin()->second == 2000 * prec);

      //test reduce releasing
      balance_lock_update({ u_2000_private_key }, u_2000_id, 4000 * prec);
      BOOST_CHECK(pledge_balance_obj5.pledge == 4000 * prec);
      BOOST_CHECK(pledge_balance_obj5.total_releasing_pledge == 4000 * prec);
      BOOST_CHECK(pledge_balance_obj5.releasing_pledges.size() == 2);
      BOOST_CHECK(pledge_balance_obj5.releasing_pledges.rbegin()->first == db.head_block_num() + GRAPHENE_DEFAULT_UNLOCKED_BALANCE_RELEASE_DELAY);
      BOOST_CHECK(pledge_balance_obj5.releasing_pledges.rbegin()->second == 1000 * prec);

      generate_blocks(GRAPHENE_DEFAULT_UNLOCKED_BALANCE_RELEASE_DELAY);
      generate_blocks(5);

      const auto& user6 = db.get_account_statistics_by_uid(u_1000_id);
      const auto& pledge_balance_obj6 = user6.pledge_balance_ids.at(pledge_balance_type::Lock_balance)(db);
      BOOST_CHECK(pledge_balance_obj6.pledge == 6000 * prec);
      BOOST_CHECK(pledge_balance_obj6.releasing_pledges.size() == 0);
      BOOST_CHECK(pledge_balance_obj6.total_releasing_pledge == 0);
      const auto& user7 = db.get_account_statistics_by_uid(u_2000_id);
      const auto& pledge_balance_obj7 = user7.pledge_balance_ids.at(pledge_balance_type::Lock_balance)(db);
      BOOST_CHECK(pledge_balance_obj7.pledge == 4000 * prec);
      BOOST_CHECK(pledge_balance_obj7.releasing_pledges.size() == 0);
      BOOST_CHECK(pledge_balance_obj7.total_releasing_pledge == 0);

   }
   catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(total_witness_pledge_test)
{
   try{
      ACTORS((1000)(2000)(9001)(9002)(9003));

      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };
      transfer(committee_account, u_9001_id, _core(10000));
      transfer(committee_account, u_9002_id, _core(10000));
      transfer(committee_account, u_9003_id, _core(10000));
      add_csaf_for_account(u_9001_id, 10000);
      add_csaf_for_account(u_9002_id, 10000);
      add_csaf_for_account(u_9003_id, 10000);
      create_witness(u_9001_id, u_9001_private_key, _core(10000));
      create_witness(u_9002_id, u_9002_private_key, _core(10000));
      create_witness(u_9003_id, u_9003_private_key, _core(10000));


      transfer(committee_account, u_1000_id, _core(30000));
      transfer(committee_account, u_2000_id, _core(30000));
      add_csaf_for_account(u_1000_id, 10000);
      add_csaf_for_account(u_2000_id, 10000);

      const witness_object& witness1 = create_witness(u_1000_id, u_1000_private_key, _core(10000));
      const witness_object& witness2 = create_witness(u_2000_id, u_2000_private_key, _core(10000));
      BOOST_CHECK(witness1.pledge == 10000 * prec);
      BOOST_CHECK(witness2.pledge == 10000 * prec);

      const dynamic_global_property_object dpo = db.get_dynamic_global_properties();
      BOOST_CHECK(dpo.total_witness_pledge == 50000 * prec);
      BOOST_CHECK(dpo.resign_witness_pledge_before_05 == 0);

      update_witness({ u_1000_private_key }, u_1000_id, optional<public_key_type>(), _core(15000), optional<string>());
      update_witness({ u_2000_private_key }, u_2000_id, optional<public_key_type>(), _core(15000), optional<string>());
      generate_blocks(1);

      const dynamic_global_property_object dpo1 = db.get_dynamic_global_properties();
      BOOST_CHECK(dpo1.total_witness_pledge == 60000 * prec);
      BOOST_CHECK(dpo1.resign_witness_pledge_before_05 == 0);

      update_witness({ u_1000_private_key }, u_1000_id, optional<public_key_type>(), _core(0), optional<string>());
      update_witness({ u_2000_private_key }, u_2000_id, optional<public_key_type>(), _core(20000), optional<string>());
      const dynamic_global_property_object dpo2 = db.get_dynamic_global_properties();
      BOOST_CHECK(dpo2.total_witness_pledge == 65000 * prec);
      BOOST_CHECK(dpo2.resign_witness_pledge_before_05 == (-15000)*prec);

      generate_blocks(28800);
      const dynamic_global_property_object dpo3 = db.get_dynamic_global_properties();
      BOOST_CHECK(dpo3.total_witness_pledge == 65000 * prec);
      BOOST_CHECK(dpo3.resign_witness_pledge_before_05 == (-15000)*prec);
   }
   catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(csaf_compute_test)
{
   try{
      ACTORS((1000)(2000)(3000));

      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };

      transfer(committee_account, u_1000_id, _core(100000000));
      transfer(committee_account, u_3000_id, _core(100000));
      collect_csaf_from_committee(u_1000_id, 100);
      collect_csaf_from_committee(u_3000_id, 100);

      //before_hardfork_04
      csaf_lease({ u_1000_private_key }, u_1000_id, u_2000_id, 15000, db.head_block_time() + 1000000);
      csaf_lease({ u_3000_private_key }, u_3000_id, u_1000_id, 10000, db.head_block_time() + 1000000);

      generate_blocks(10);
      collect_csaf_origin({ u_1000_private_key }, u_1000_id, u_1000_id, 1);
      const _account_statistics_object& ants1_1000 = db.get_account_statistics_by_uid(u_1000_id);
      time_point_sec now1 = db.head_block_time();
      fc::time_point_sec now_rounded1((now1.sec_since_epoch() / 60) * 60);
      share_type effective_balance1 = ants1_1000.core_balance + ants1_1000.core_leased_in - ants1_1000.core_leased_out;
      auto coin_seconds_earned1 = compute_coin_seconds_earned(ants1_1000, effective_balance1, now_rounded1);
      BOOST_CHECK(ants1_1000.coin_seconds_earned == coin_seconds_earned1);

      //after_hardfork_04
      generate_blocks(HARDFORK_0_4_TIME, true);
      create_witness(u_1000_id, u_1000_private_key, _core(100000));

      generate_block(10);
      collect_csaf_origin({ u_1000_private_key }, u_1000_id, u_1000_id, 1);
      const _account_statistics_object& ants2_1000 = db.get_account_statistics_by_uid(u_1000_id);
      time_point_sec now2 = db.head_block_time();
      fc::time_point_sec now_rounded2((now2.sec_since_epoch() / 60) * 60);
      share_type effective_balance2 = ants2_1000.core_balance + ants2_1000.core_leased_in - ants2_1000.core_leased_out
         - ants2_1000.get_pledge_balance(GRAPHENE_CORE_ASSET_AID, pledge_balance_type::Witness, db);
      auto coin_seconds_earned2 = compute_coin_seconds_earned(ants2_1000, effective_balance2, now_rounded2);
      BOOST_CHECK(ants2_1000.coin_seconds_earned == coin_seconds_earned2);

      //after_hardfork_05
      generate_blocks(HARDFORK_0_5_TIME, true);
      balance_lock_update({ u_1000_private_key }, u_1000_id, 10000 * prec);

      generate_block(10);
      collect_csaf_origin({ u_1000_private_key }, u_1000_id, u_1000_id, 1);
      const _account_statistics_object& ants3_1000 = db.get_account_statistics_by_uid(u_1000_id);
      time_point_sec now3 = db.head_block_time();
      fc::time_point_sec now_rounded3((now3.sec_since_epoch() / 60) * 60);
      share_type effective_balance3;
      auto pledge_balance_obj = ants3_1000.pledge_balance_ids.at(pledge_balance_type::Lock_balance)(db);
      if (GRAPHENE_CORE_ASSET_AID == pledge_balance_obj.asset_id)
         effective_balance3 = pledge_balance_obj.pledge;
      auto coin_seconds_earned3 = compute_coin_seconds_earned(ants3_1000, effective_balance3, now_rounded3);

      BOOST_CHECK(ants3_1000.coin_seconds_earned == coin_seconds_earned3);
      
   }
   catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(csaf_lease_test)
{
   try{
      ACTORS((1000)(2000));

      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };

      transfer(committee_account, u_1000_id, _core(30000));
      transfer(committee_account, u_2000_id, _core(30000));
      add_csaf_for_account(u_1000_id, 10000);
      add_csaf_for_account(u_2000_id, 10000);

      csaf_lease({ u_1000_private_key }, u_1000_id, u_2000_id, 15000, db.head_block_time() + 10000);
      const _account_statistics_object ant_1000 = db.get_account_statistics_by_uid(u_1000_id);
      const _account_statistics_object ant_2000 = db.get_account_statistics_by_uid(u_2000_id);
      BOOST_CHECK(ant_1000.core_leased_out == 15000*prec);
      BOOST_CHECK(ant_2000.core_leased_in == 15000 * prec);

      generate_blocks(28800);
      const _account_statistics_object ant_1000a = db.get_account_statistics_by_uid(u_1000_id);
      const _account_statistics_object ant_2000a = db.get_account_statistics_by_uid(u_2000_id);
      BOOST_CHECK(ant_1000a.core_leased_out == 0 * prec);
      BOOST_CHECK(ant_2000a.core_leased_in == 0 * prec);

   }
   catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(limit_order_test)
{
   try{
      ACTORS((1000)(2000)(3000)(1001)(1002)(2001)(2002)(3001)(3002));

      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };

      auto balance_u_1000_0=_core(30000);
      auto balance_u_2000_0=_core(30000);
      auto balance_u_3000_0=_core(30000);
      
      transfer(committee_account, u_1000_id, balance_u_1000_0);
      transfer(committee_account, u_2000_id, balance_u_2000_0);
      transfer(committee_account, u_3000_id, balance_u_3000_0);
      add_csaf_for_account(u_1000_id, 10000);
      add_csaf_for_account(u_2000_id, 10000);
      add_csaf_for_account(u_3000_id, 10000);
      generate_blocks(HARDFORK_0_5_TIME, true);
      
      auto registrar_percent=60*GRAPHENE_1_PERCENT;
      auto referrer_percent = 40*GRAPHENE_1_PERCENT;
      
      const account_object& ant_obj1= db.get_account_by_uid(u_1000_id);
      db.modify(ant_obj1, [&](account_object& s) {
         s.reg_info.registrar = u_1001_id;
         s.reg_info.referrer = u_1002_id;
         s.reg_info.registrar_percent =registrar_percent;
         s.reg_info.referrer_percent = referrer_percent;
      });
      const account_object& ant_obj2 = db.get_account_by_uid(u_2000_id);
      db.modify(ant_obj2, [&](account_object& s) {
         s.reg_info.registrar = u_2001_id;
         s.reg_info.referrer = u_2002_id;
         s.reg_info.registrar_percent = registrar_percent;
         s.reg_info.referrer_percent = referrer_percent;
      });
      const account_object& ant_obj3 = db.get_account_by_uid(u_3000_id);
      db.modify(ant_obj3, [&](account_object& s) {
         s.reg_info.registrar = u_3001_id;
         s.reg_info.referrer = u_3002_id;
         s.reg_info.registrar_percent = registrar_percent;
         s.reg_info.referrer_percent = referrer_percent;
      });

      auto market_fee_percent=1*GRAPHENE_1_PERCENT;
      auto market_reward_percent = 50 * GRAPHENE_1_PERCENT;
      auto max_supply = 100000000*prec;
      auto max_market_fee_1=20*prec;
      auto max_market_fee_2=2000*prec;
      asset_options options;
      options.max_supply = max_supply;
      options.market_fee_percent = market_fee_percent;
      options.max_market_fee = max_market_fee_1;
      options.issuer_permissions = 15;
      options.flags = charge_market_fee;
      //options.whitelist_authorities = ;
      //options.blacklist_authorities = ;
      //options.whitelist_markets = ;
      //options.blacklist_markets = ;
      options.description = "test asset";
      options.extensions = graphene::chain::extension<additional_asset_options>();
      additional_asset_options exts;
      exts.reward_percent = market_reward_percent;
      options.extensions->value = exts;

      const account_object commit_obj = db.get_account_by_uid(committee_account);
      
      auto initial_supply=share_type(100000000 * prec);
      
      create_asset({ u_1000_private_key }, u_1000_id, "ABC", 5, options, initial_supply);
      const asset_object& ast = db.get_asset_by_aid(1);
      BOOST_CHECK(ast.symbol == "ABC");
      BOOST_CHECK(ast.precision == 5);
      BOOST_CHECK(ast.issuer == u_1000_id);
      BOOST_CHECK(ast.options.max_supply == max_supply);
      BOOST_CHECK(ast.options.flags == charge_market_fee);
      asset ast1 = db.get_balance(u_1000_id, 1);
      BOOST_CHECK(ast1.asset_id == 1);
      BOOST_CHECK(ast1.amount == initial_supply);

      auto options2=options;
      options2.max_market_fee = max_market_fee_2;
      create_asset({ u_3000_private_key }, u_3000_id, "CBA", 5, options2, initial_supply);
      const asset_object& ast2 = db.get_asset_by_aid(2);
      BOOST_CHECK(ast2.symbol == "CBA");
      BOOST_CHECK(ast2.precision == 5);
      BOOST_CHECK(ast2.issuer == u_3000_id);
      BOOST_CHECK(ast2.options.max_supply == max_supply);
      BOOST_CHECK(ast2.options.flags == charge_market_fee);
      asset ast7 = db.get_balance(u_3000_id, 2);
      BOOST_CHECK(ast7.asset_id == 2);
      BOOST_CHECK(ast7.amount == initial_supply);
      
      
      //full match market pair asset_0 and asset_1
      auto expiration_time=fc::time_point::now().sec_since_epoch()+24*3600;
      auto sell_asset_1_amount=10000*prec;
      auto sell_asset_0_amount=1000*prec;
      
      auto balance_u_1000_1=initial_supply;
      auto balance_u_2000_1=share_type(0);
      auto balance_u_3000_2=initial_supply;
      
      create_limit_order({ u_1000_private_key }, u_1000_id, 1, sell_asset_1_amount, 0, sell_asset_0_amount, expiration_time, false);
      create_limit_order({ u_2000_private_key }, u_2000_id, 0, sell_asset_0_amount, 1, sell_asset_1_amount, expiration_time, false);


      asset ast1000_1 = db.get_balance(u_1000_id, 1);
      BOOST_CHECK(ast1000_1.asset_id == 1);
      balance_u_1000_1=balance_u_1000_1-sell_asset_1_amount;
      BOOST_CHECK(ast1000_1.amount ==balance_u_1000_1);
      
      asset ast2000_1 = db.get_balance(u_2000_id, 1);
      BOOST_CHECK(ast2000_1.asset_id == 1);
      auto market_fee_asset_1=sell_asset_1_amount*market_fee_percent/GRAPHENE_100_PERCENT;
      if(market_fee_asset_1>max_market_fee_1)
         market_fee_asset_1=max_market_fee_1;
      balance_u_2000_1=sell_asset_1_amount-market_fee_asset_1;
      BOOST_CHECK(ast2000_1.amount ==balance_u_2000_1);
      
      auto registrar_r_1=(market_fee_asset_1*market_reward_percent/GRAPHENE_100_PERCENT)*registrar_percent/GRAPHENE_100_PERCENT;
      auto referrer_r_1=(market_fee_asset_1*market_reward_percent/GRAPHENE_100_PERCENT)*referrer_percent/GRAPHENE_100_PERCENT;
      auto asset_1_claim_fees=market_fee_asset_1*(GRAPHENE_100_PERCENT-market_reward_percent)/GRAPHENE_100_PERCENT;
      
      const _account_statistics_object& aso1 = db.get_account_statistics_by_uid(u_2001_id);
      auto iter1 = aso1.uncollected_market_fees.find(1);
      BOOST_CHECK(iter1 != aso1.uncollected_market_fees.end());
      BOOST_CHECK(iter1->second == registrar_r_1);
      const _account_statistics_object& aso2 = db.get_account_statistics_by_uid(u_2002_id);
      auto iter2 = aso2.uncollected_market_fees.find(1);
      BOOST_CHECK(iter2 != aso2.uncollected_market_fees.end());
      BOOST_CHECK(iter2->second ==referrer_r_1);
      
      const asset_dynamic_data_object& ast_dy_obj = db.get_asset_by_aid(1).dynamic_asset_data_id(db);
      BOOST_CHECK(ast_dy_obj.accumulated_fees == asset_1_claim_fees);

      collect_market_fee({ u_2001_private_key }, u_2001_id, 1, registrar_r_1);
      collect_market_fee({ u_2002_private_key }, u_2002_id, 1, referrer_r_1);
      asset ast4 = db.get_balance(u_2001_id, 1);
      BOOST_CHECK(ast4.asset_id == 1);
      BOOST_CHECK(ast4.amount == 6 * prec);
      asset ast5 = db.get_balance(u_2002_id, 1);
      BOOST_CHECK(ast5.asset_id == 1);
      BOOST_CHECK(ast5.amount == 4 * prec);

      asset_claim_fees({u_1000_private_key}, u_1000_id, 1, asset_1_claim_fees);
      const asset_dynamic_data_object& ast_dy_obj2 = db.get_asset_by_aid(1).dynamic_asset_data_id(db);
      BOOST_CHECK(ast_dy_obj2.accumulated_fees == 0);
      ast1000_1 = db.get_balance(u_1000_id, 1);
      balance_u_1000_1+=asset_1_claim_fees;
      BOOST_CHECK(ast1000_1.asset_id == 1);
      BOOST_CHECK(ast1000_1.amount == balance_u_1000_1);

      ///////////////////////////////////////////////////////////////////////////////////////

      sell_asset_1_amount=10000*prec;
      auto sell_asset_2_amount=20000 * prec;
      create_limit_order({ u_1000_private_key }, u_1000_id, 1, sell_asset_1_amount, 2, sell_asset_2_amount, expiration_time, false);
      create_limit_order({ u_3000_private_key }, u_3000_id, 2, sell_asset_2_amount, 1, sell_asset_1_amount, expiration_time, false);

      ast1000_1 = db.get_balance(u_1000_id, 1);
      BOOST_CHECK(ast1000_1.asset_id == 1);
      balance_u_1000_1=balance_u_1000_1-sell_asset_1_amount;
      
      BOOST_CHECK(ast1000_1.amount == balance_u_1000_1);
      auto ast1000_2= db.get_balance(u_1000_id, 2);
      BOOST_CHECK(ast1000_2.asset_id == 2);
      
      // to check overflow
      auto market_fee_asset_2=sell_asset_2_amount*market_fee_percent/GRAPHENE_100_PERCENT;
      if(market_fee_asset_2>max_market_fee_2)
         market_fee_asset_2=max_market_fee_2;
      auto balance_u_1000_2=sell_asset_2_amount-market_fee_asset_2;
      BOOST_CHECK(ast1000_2.amount == balance_u_1000_2);

      asset ast3000_1 = db.get_balance(u_3000_id, 1);
      BOOST_CHECK(ast3000_1.asset_id == 1);
      
      market_fee_asset_1=sell_asset_1_amount*market_fee_percent/GRAPHENE_100_PERCENT;
      if(market_fee_asset_1>max_market_fee_1)
         market_fee_asset_1=max_market_fee_1;
      auto balance_u_3000_1=sell_asset_1_amount-market_fee_asset_1;
      
      BOOST_CHECK(ast3000_1.amount ==balance_u_3000_1);
      
      asset ast3000_2 = db.get_balance(u_3000_id, 2);
      BOOST_CHECK(ast3000_2.asset_id == 2);
      balance_u_3000_2-=sell_asset_2_amount;
      BOOST_CHECK(ast3000_2.amount == balance_u_3000_2);

      const asset_dynamic_data_object& ast_1_dy_obj = db.get_asset_by_aid(1).dynamic_asset_data_id(db);
      asset_1_claim_fees=market_fee_asset_1*(GRAPHENE_100_PERCENT-market_reward_percent)/GRAPHENE_100_PERCENT;
      BOOST_CHECK(ast_1_dy_obj.accumulated_fees == asset_1_claim_fees);
      
      auto asset_2_claim_fees=market_fee_asset_2*(GRAPHENE_100_PERCENT-market_reward_percent)/GRAPHENE_100_PERCENT;
      const asset_dynamic_data_object& ast_2_dy_obj = db.get_asset_by_aid(2).dynamic_asset_data_id(db);
      BOOST_CHECK(ast_2_dy_obj.accumulated_fees == asset_2_claim_fees);

      registrar_r_1=(market_fee_asset_1*market_reward_percent/GRAPHENE_100_PERCENT)*registrar_percent/GRAPHENE_100_PERCENT;
      referrer_r_1=(market_fee_asset_1*market_reward_percent/GRAPHENE_100_PERCENT)*referrer_percent/GRAPHENE_100_PERCENT;
      
      auto registrar_r_2=(market_fee_asset_2*market_reward_percent/GRAPHENE_100_PERCENT)*registrar_percent/GRAPHENE_100_PERCENT;
      auto referrer_r_2=(market_fee_asset_2*market_reward_percent/GRAPHENE_100_PERCENT)*referrer_percent/GRAPHENE_100_PERCENT;
      
      const _account_statistics_object& aso3 = db.get_account_statistics_by_uid(u_3001_id);
      auto iter3 = aso3.uncollected_market_fees.find(1);
      BOOST_CHECK(iter3 != aso3.uncollected_market_fees.end());
      BOOST_CHECK(iter3->second == registrar_r_1);
      const _account_statistics_object& aso4 = db.get_account_statistics_by_uid(u_3002_id);
      auto iter4 = aso4.uncollected_market_fees.find(1);
      BOOST_CHECK(iter4 != aso4.uncollected_market_fees.end());
      BOOST_CHECK(iter4->second == referrer_r_1);

      const _account_statistics_object& aso5 = db.get_account_statistics_by_uid(u_1001_id);
      auto iter5 = aso5.uncollected_market_fees.find(2);
      BOOST_CHECK(iter5 != aso5.uncollected_market_fees.end());
      BOOST_CHECK(iter5->second == registrar_r_2);
      const _account_statistics_object& aso6 = db.get_account_statistics_by_uid(u_1002_id);
      auto iter6 = aso6.uncollected_market_fees.find(2);
      BOOST_CHECK(iter6 != aso6.uncollected_market_fees.end());
      BOOST_CHECK(iter6->second == referrer_r_2);
      
   }
   catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(limit_order_test2)
{
   try{
      ACTORS((1000)(2000)(3000)(4000));

      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };

      auto balance_u_1000_0 = _core(30000);
      auto balance_u_2000_0 = _core(30000);
      auto balance_u_3000_0 = _core(30000);
      auto balance_u_4000_0 = _core(30000);

      transfer(committee_account, u_1000_id, balance_u_1000_0);
      transfer(committee_account, u_2000_id, balance_u_2000_0);
      transfer(committee_account, u_3000_id, balance_u_3000_0);
      transfer(committee_account, u_4000_id, balance_u_4000_0);
      add_csaf_for_account(u_1000_id, 10000);
      add_csaf_for_account(u_2000_id, 10000);
      add_csaf_for_account(u_3000_id, 10000);
      add_csaf_for_account(u_4000_id, 10000);
      generate_blocks(HARDFORK_0_5_TIME, true);

      auto market_fee_percent = 1 * GRAPHENE_1_PERCENT;
      auto market_reward_percent = 50 * GRAPHENE_1_PERCENT;
      auto max_supply = 100000000 * prec;
      auto max_market_fee_1 = 20 * prec;
      auto max_market_fee_2 = 2000 * prec;
      asset_options options;
      options.max_supply = max_supply;
      options.market_fee_percent = market_fee_percent;
      options.max_market_fee = max_market_fee_1;
      options.issuer_permissions = 15;
      options.flags = charge_market_fee;
      //options.whitelist_authorities = ;
      //options.blacklist_authorities = ;
      //options.whitelist_markets = ;
      //options.blacklist_markets = ;
      options.description = "test asset";
      options.extensions = graphene::chain::extension<additional_asset_options>();
      additional_asset_options exts;
      exts.reward_percent = market_reward_percent;
      options.extensions->value = exts;

      const account_object commit_obj = db.get_account_by_uid(committee_account);
      auto initial_supply = share_type(100000000 * prec);
      create_asset({ u_1000_private_key }, u_1000_id, "ABC", 5, options, initial_supply);
      const asset_object& ast = db.get_asset_by_aid(1);
      BOOST_CHECK(ast.symbol == "ABC");
      BOOST_CHECK(ast.precision == 5);
      BOOST_CHECK(ast.issuer == u_1000_id);
      BOOST_CHECK(ast.options.max_supply == max_supply);
      BOOST_CHECK(ast.options.flags == charge_market_fee);
      asset ast1 = db.get_balance(u_1000_id, 1);
      BOOST_CHECK(ast1.asset_id == 1);
      BOOST_CHECK(ast1.amount == initial_supply);

      auto options2 = options;
      options2.max_market_fee = max_market_fee_2;
      create_asset({ u_2000_private_key }, u_2000_id, "CBA", 5, options2, initial_supply);
      const asset_object& ast2 = db.get_asset_by_aid(2);
      BOOST_CHECK(ast2.symbol == "CBA");
      BOOST_CHECK(ast2.precision == 5);
      BOOST_CHECK(ast2.issuer == u_2000_id);
      BOOST_CHECK(ast2.options.max_supply == max_supply);
      BOOST_CHECK(ast2.options.flags == charge_market_fee);
      asset ast7 = db.get_balance(u_2000_id, 2);
      BOOST_CHECK(ast7.asset_id == 2);
      BOOST_CHECK(ast7.amount == initial_supply);


      //test for less match
      auto expiration_time = fc::time_point::now().sec_since_epoch() + 24 * 3600;
      auto sell_asset_1_amount = 10000 * prec;
      auto sell_asset_0_amount = 1000 * prec;

      //u_1000 :  10000 ABC <---> 1000 YOYO
      create_limit_order({ u_1000_private_key }, u_1000_id, 1, sell_asset_1_amount, 0, sell_asset_0_amount, expiration_time, false);
      //u_3000 :  500 YOYO  <---> 5000 ABC
      create_limit_order({ u_3000_private_key }, u_3000_id, 0, sell_asset_0_amount/2, 1, sell_asset_1_amount/2, expiration_time, false);

      auto fee1 = (sell_asset_1_amount / 2)* market_fee_percent / GRAPHENE_100_PERCENT;
      if (fee1 > max_market_fee_1) fee1 = max_market_fee_1;

      asset ast1000_1 = db.get_balance(u_1000_id, 1);
      BOOST_CHECK(ast1000_1.asset_id == 1);
      auto balance_u_1000_1 = initial_supply - sell_asset_1_amount;
      BOOST_CHECK(ast1000_1.amount == balance_u_1000_1);

      asset ast3000_1 = db.get_balance(u_3000_id, 1);
      BOOST_CHECK(ast3000_1.asset_id == 1);
      auto balance_u_3000_1 = sell_asset_1_amount / 2 - fee1;
      BOOST_CHECK(ast3000_1.amount == balance_u_3000_1);

      //u_4000 :  500 YOYO  <---> 5000 ABC
      create_limit_order({ u_4000_private_key }, u_4000_id, 0, sell_asset_0_amount / 2, 1, sell_asset_1_amount / 2, expiration_time, false);

      auto fee2 = (sell_asset_1_amount / 2)* market_fee_percent / GRAPHENE_100_PERCENT;
      if (fee2 > max_market_fee_1) fee2 = max_market_fee_1;

      asset ast1000_2 = db.get_balance(u_1000_id, 1);
      BOOST_CHECK(ast1000_2.asset_id == 1);
      auto balance_u_1000_2 = initial_supply - sell_asset_1_amount / 2 - sell_asset_1_amount / 2;
      BOOST_CHECK(ast1000_2.amount == balance_u_1000_2);

      asset ast3000_2 = db.get_balance(u_3000_id, 1);
      BOOST_CHECK(ast3000_2.asset_id == 1);
      auto balance_u_3000_2 = sell_asset_1_amount / 2 - fee2;
      BOOST_CHECK(ast3000_2.amount == balance_u_3000_2);

      //test for less match 2

      //u_1000 :  10000 ABC <---> 1000 CBA
      create_limit_order({ u_1000_private_key }, u_1000_id, 1, sell_asset_1_amount, 2, sell_asset_0_amount, expiration_time, false);
      //u_2000 :  500 CBA  <---> 5000 ABC
      create_limit_order({ u_2000_private_key }, u_2000_id, 2, sell_asset_0_amount / 2, 1, sell_asset_1_amount / 2, expiration_time, false);

      auto fee3 = (sell_asset_0_amount / 2)* market_fee_percent / GRAPHENE_100_PERCENT;
      if (fee3 > max_market_fee_2) fee3 = max_market_fee_2;

      asset ast1000_3 = db.get_balance(u_1000_id, 2);
      BOOST_CHECK(ast1000_3.asset_id == 2);
      auto balance_u_1000_3 = sell_asset_0_amount / 2 - fee3;
      BOOST_CHECK(ast1000_3.amount == balance_u_1000_3);

      auto fee4 = (sell_asset_1_amount / 2)* market_fee_percent / GRAPHENE_100_PERCENT;
      if (fee4 > max_market_fee_1) fee4 = max_market_fee_1;

      asset ast2000_1 = db.get_balance(u_2000_id, 1);
      BOOST_CHECK(ast2000_1.asset_id == 1);
      auto balance_u_2000_1 = sell_asset_1_amount / 2 - fee4;
      BOOST_CHECK(ast2000_1.amount == balance_u_3000_1);

      //u_2000 :  200 YOYO  <---> 2000 ABC
      create_limit_order({ u_2000_private_key }, u_2000_id, 2, sell_asset_0_amount / 5, 1, sell_asset_1_amount / 5, expiration_time, false);

      auto fee5 = (sell_asset_0_amount / 5)* market_fee_percent / GRAPHENE_100_PERCENT;
      if (fee5 > max_market_fee_2) fee5 = max_market_fee_2;

      asset ast1000_4 = db.get_balance(u_1000_id, 2);
      BOOST_CHECK(ast1000_4.asset_id == 2);
      auto balance_u_1000_4 = balance_u_1000_3 + sell_asset_0_amount / 5 - fee5;
      BOOST_CHECK(ast1000_4.amount == balance_u_1000_4);

      auto fee6 = (sell_asset_1_amount / 5)* market_fee_percent / GRAPHENE_100_PERCENT;
      if (fee6 > max_market_fee_1) fee6 = max_market_fee_1;

      asset ast2000_2 = db.get_balance(u_2000_id, 1);
      BOOST_CHECK(ast2000_2.asset_id == 1);
      auto balance_u_2000_2 = balance_u_2000_1 + sell_asset_1_amount / 5 - fee6;
      BOOST_CHECK(ast2000_2.amount == balance_u_2000_2);
   }
   catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(limit_order_test3_for_votes)
{
   try{
      ACTORS((1000)(2000)(3000)(4000));

      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };

      auto balance_u_1000_0 = _core(30000);
      auto balance_u_2000_0 = _core(30000);
      auto balance_u_3000_0 = _core(30000);
      auto balance_u_4000_0 = _core(30000);

      transfer(committee_account, u_1000_id, balance_u_1000_0);
      transfer(committee_account, u_2000_id, balance_u_2000_0);
      transfer(committee_account, u_3000_id, balance_u_3000_0);
      transfer(committee_account, u_4000_id, balance_u_4000_0);
      add_csaf_for_account(u_1000_id, 10000);
      add_csaf_for_account(u_2000_id, 10000);
      add_csaf_for_account(u_3000_id, 10000);
      add_csaf_for_account(u_4000_id, 10000);
      generate_blocks(HARDFORK_0_5_TIME, true);

      auto market_fee_percent = 1 * GRAPHENE_1_PERCENT;
      auto market_reward_percent = 50 * GRAPHENE_1_PERCENT;
      auto max_supply = 100000000 * prec;
      auto max_market_fee_1 = 20 * prec;
      auto max_market_fee_2 = 2000 * prec;
      asset_options options;
      options.max_supply = max_supply;
      options.market_fee_percent = market_fee_percent;
      options.max_market_fee = max_market_fee_1;
      options.issuer_permissions = 15;
      options.flags = charge_market_fee;
      //options.whitelist_authorities = ;
      //options.blacklist_authorities = ;
      //options.whitelist_markets = ;
      //options.blacklist_markets = ;
      options.description = "test asset";
      options.extensions = graphene::chain::extension<additional_asset_options>();
      additional_asset_options exts;
      exts.reward_percent = market_reward_percent;
      options.extensions->value = exts;

      const account_object commit_obj = db.get_account_by_uid(committee_account);
      auto initial_supply = share_type(100000000 * prec);
      create_asset({ u_1000_private_key }, u_1000_id, "ABC", 5, options, initial_supply);
      const asset_object& ast = db.get_asset_by_aid(1);
      BOOST_CHECK(ast.symbol == "ABC");
      BOOST_CHECK(ast.precision == 5);
      BOOST_CHECK(ast.issuer == u_1000_id);
      BOOST_CHECK(ast.options.max_supply == max_supply);
      BOOST_CHECK(ast.options.flags == charge_market_fee);
      asset ast1 = db.get_balance(u_1000_id, 1);
      BOOST_CHECK(ast1.asset_id == 1);
      BOOST_CHECK(ast1.amount == initial_supply);

      auto options2 = options;
      options2.max_market_fee = max_market_fee_2;
      create_asset({ u_2000_private_key }, u_2000_id, "CBA", 5, options2, initial_supply);
      const asset_object& ast2 = db.get_asset_by_aid(2);
      BOOST_CHECK(ast2.symbol == "CBA");
      BOOST_CHECK(ast2.precision == 5);
      BOOST_CHECK(ast2.issuer == u_2000_id);
      BOOST_CHECK(ast2.options.max_supply == max_supply);
      BOOST_CHECK(ast2.options.flags == charge_market_fee);
      asset ast7 = db.get_balance(u_2000_id, 2);
      BOOST_CHECK(ast7.asset_id == 2);
      BOOST_CHECK(ast7.amount == initial_supply);

      flat_set<account_uid_type> platform_set1;
      platform_set1.insert(u_4000_id);
      flat_set<account_uid_type> platform_set2;
      create_platform(u_4000_id, "platform", _core(10000), "www.123456789.com", "", { u_4000_private_key });
      update_platform_votes(u_1000_id, platform_set1, platform_set2, { u_1000_private_key });
      update_platform_votes(u_2000_id, platform_set1, platform_set2, { u_2000_private_key });
      update_platform_votes(u_3000_id, platform_set1, platform_set2, { u_3000_private_key });

      //other asset for core asset
      auto expiration_time = fc::time_point::now().sec_since_epoch() + 24 * 3600;
      auto sell_asset_1_amount = 10000 * prec;
      auto sell_asset_0_amount = 1000 * prec;

      share_type core_balance1_1 = db.get_account_statistics_by_uid(u_1000_id).core_balance;
      //u_1000 :  10000 ABC <---> 1000 YOYO
      create_limit_order({ u_1000_private_key }, u_1000_id, 1, sell_asset_1_amount, 0, sell_asset_0_amount, expiration_time, false);

      auto account1_1 = db.get_account_statistics_by_uid(u_1000_id);
      BOOST_CHECK(account1_1.total_core_in_orders == 0);

      uint64_t votes1_1 = account1_1.get_votes_from_core_balance();
      const voter_object* voter1 = db.find_voter(u_1000_id, account1_1.last_voter_sequence);
      BOOST_CHECK(voter1 != nullptr);
      BOOST_CHECK(voter1->votes == votes1_1);
      BOOST_CHECK(votes1_1 == core_balance1_1.value);

      //u_3000 :  500 YOYO  <---> 5000 ABC
      share_type core_balance3_1 = db.get_account_statistics_by_uid(u_3000_id).core_balance;
      create_limit_order({ u_3000_private_key }, u_3000_id, 0, sell_asset_0_amount / 2, 1, sell_asset_1_amount / 2, expiration_time, false);

      auto account1_2 = db.get_account_statistics_by_uid(u_1000_id);
      uint64_t votes1_2 = account1_2.get_votes_from_core_balance();
      const voter_object* voter1_2 = db.find_voter(u_1000_id, account1_2.last_voter_sequence);
      BOOST_CHECK(voter1_2 != nullptr);
      BOOST_CHECK(voter1_2->votes == votes1_2);
      BOOST_CHECK(votes1_2 == core_balance1_1.value + (sell_asset_0_amount / 2).value);

      auto account3_1 = db.get_account_statistics_by_uid(u_3000_id);
      uint64_t votes3_1 = account3_1.get_votes_from_core_balance();
      const voter_object* voter3_1 = db.find_voter(u_3000_id, account3_1.last_voter_sequence);
      BOOST_CHECK(voter3_1 != nullptr);
      BOOST_CHECK(voter3_1->votes == votes3_1);
      BOOST_CHECK(votes3_1 == core_balance3_1.value - (sell_asset_0_amount / 2).value);

      //////////////////////////////////////////
      //core asset for other asset
      //////////////////////////////////////////
      auto sell_asset_2_amount = 2000 * prec;

      share_type core_balance3_2 = db.get_account_statistics_by_uid(u_3000_id).core_balance;
      //u_3000 :  1000 YOYO <---> 2000 CBA
      create_limit_order({ u_3000_private_key }, u_3000_id, 0, sell_asset_0_amount, 2, sell_asset_2_amount, expiration_time, false);

      auto account3_2 = db.get_account_statistics_by_uid(u_3000_id);
      BOOST_CHECK(account3_2.total_core_in_orders == sell_asset_0_amount.value);

      uint64_t votes3_2 = account3_2.get_votes_from_core_balance();
      const voter_object* voter3_2 = db.find_voter(u_3000_id, account3_2.last_voter_sequence);
      BOOST_CHECK(voter3_2 != nullptr);
      BOOST_CHECK(voter3_2->votes == votes3_2);
      BOOST_CHECK(votes3_2 == core_balance3_2.value);

      //u_2000 :  4000 CBA  <---> 2000 YOYO
      share_type core_balance2_1 = db.get_account_statistics_by_uid(u_2000_id).core_balance;
      create_limit_order({ u_2000_private_key }, u_2000_id, 2, sell_asset_2_amount * 2, 0, sell_asset_0_amount * 2, expiration_time, false);

      const _account_statistics_object& account3_3 = db.get_account_statistics_by_uid(u_3000_id);
      uint64_t votes3_3 = account3_3.get_votes_from_core_balance();
      const voter_object* voter3_3 = db.find_voter(u_3000_id, account3_3.last_voter_sequence);
      BOOST_CHECK(voter3_3 != nullptr);
      BOOST_CHECK(voter3_3->votes == votes3_3);
      BOOST_CHECK(votes3_3 == core_balance3_2.value - sell_asset_0_amount.value);


      const _account_statistics_object& account2_1 = db.get_account_statistics_by_uid(u_2000_id);
      uint64_t votes2_1 = account2_1.get_votes_from_core_balance();
      const voter_object* voter2_1 = db.find_voter(u_2000_id, account2_1.last_voter_sequence);
      BOOST_CHECK(voter2_1 != nullptr);
      BOOST_CHECK(voter2_1->votes == votes2_1);
      BOOST_CHECK(votes2_1 == core_balance2_1.value + sell_asset_0_amount.value);
   }
   catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(pledge_mining_test_1)
{
   try{
         ACTORS((1000)(2000)(3001));

         const auto& core_asset = db.get_core_asset();
         const auto& core_dyn_data = core_asset.dynamic_data(db);

         const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
         auto _core = [&](int64_t x) -> asset
         {  return asset(x*prec);    };
         transfer(committee_account, u_1000_id, _core(600000));
         transfer(committee_account, u_2000_id, _core(600000));

         transfer(committee_account, u_3001_id, _core(600000));

         add_csaf_for_account(u_1000_id, 10000);
         add_csaf_for_account(u_2000_id, 10000);
         generate_blocks(HARDFORK_0_5_TIME, true);

         //test create witness extension
         graphene::chain::pledge_mining::ext ext;
         ext.can_pledge = true;
         ext.bonus_rate = 6000;
         create_witness({ u_1000_private_key }, u_1000_id, "test pledge witness-1", 500000 * prec, u_1000_public_key, ext);
         //create_witness({ u_2000_private_key }, u_2000_id, "test pledge witness-2", 500000 * prec, u_2000_public_key);

         //check witness object
         auto witness_obj = db.get_witness_by_uid(u_1000_id);
         BOOST_CHECK(witness_obj.can_pledge == true);
         BOOST_CHECK(witness_obj.bonus_rate == 6000);

         update_mining_pledge({ u_3001_private_key }, u_3001_id, u_1000_id, 200000 * prec.value);
         generate_blocks(21);
         //check pledge mining object
         const pledge_mining_object& pledge_mining_obj = db.get_pledge_mining(u_1000_id, u_3001_id);
         const auto& pledge_balance_obj = pledge_mining_obj.pledge_id(db);
         BOOST_CHECK(pledge_balance_obj.pledge == 200000 * prec);

         auto last_pledge_witness_pay = db.get_dynamic_global_properties().by_pledge_witness_pay_per_block;

         uint32_t last_produce_blocks = 0;
         share_type total_bonus = 0;
         share_type witness_pay = 0;
         for (int i = 0; i < 10; ++i)
         {
            auto wit = db.get_witness_by_uid(u_1000_id);
            auto need_block_num = wit.last_update_bonus_block_num + 10000 - db.head_block_num();
            generate_blocks(need_block_num);
            wit = db.get_witness_by_uid(u_1000_id);
            const dynamic_global_property_object& dpo = db.get_dynamic_global_properties();

            share_type pledge_bonus = ((fc::bigint)dpo.by_pledge_witness_pay_per_block.value * wit.bonus_rate * wit.total_mining_pledge
               / ((wit.pledge + wit.total_mining_pledge) * GRAPHENE_100_PERCENT)).to_int64();
            auto produce_blocks_per_cycle = wit.total_produced - last_produce_blocks;
            last_produce_blocks = wit.total_produced;

            share_type bonus_per_pledge = ((fc::uint128_t)(produce_blocks_per_cycle * pledge_bonus).value * GRAPHENE_PLEDGE_BONUS_PRECISION
               / wit.total_mining_pledge).to_uint64();

            total_bonus += ((fc::uint128_t)bonus_per_pledge.value * pledge_balance_obj.pledge.value
               / GRAPHENE_PLEDGE_BONUS_PRECISION).to_uint64();
            auto account = db.get_account_statistics_by_uid(u_3001_id);
            BOOST_CHECK(account.uncollected_pledge_bonus == total_bonus);
            auto witness = db.get_account_statistics_by_uid(u_1000_id);
            witness_pay = dpo.by_pledge_witness_pay_per_block*wit.total_produced - total_bonus;
            BOOST_CHECK(witness.uncollected_witness_pay == witness_pay);
         }

         //test update mining pledge
         flat_map<account_uid_type, fc::ecc::private_key> mining_map;
         actor(8001, 20, mining_map);
         for (const auto& m : mining_map)
         {
            transfer(committee_account, m.first, _core(600000));
            add_csaf_for_account(m.first, 10000);
            update_mining_pledge({ m.second }, m.first, u_1000_id, 200000 * prec.value);
         }


         auto wit = db.get_witness_by_uid(u_1000_id);
         auto produce_block = wit.total_produced;
         bool is_produce_block = false;
         for (int i = 0; i < 20; ++i)
         {
            generate_block();
            const dynamic_global_property_object& dpo = db.get_dynamic_global_properties();
            if (last_pledge_witness_pay != dpo.by_pledge_witness_pay_per_block)
            {
               auto witness = db.get_witness_by_uid(u_1000_id);
               if (produce_block != db.get_witness_by_uid(u_1000_id).total_produced)
                  is_produce_block = true;
               break;
            }
         }


         //more account pledge mining to witness
         share_type total_bonus2 = 0;
         share_type witness_pay2 = 0;
         share_type bonus_3001 = 0;
         vector<share_type> total_bonus_per_account(mining_map.size());
         for (int i = 0; i < 10; ++i)
         {
            auto wit = db.get_witness_by_uid(u_1000_id);
            auto need_block_num = wit.last_update_bonus_block_num + 10000 - db.head_block_num();
            generate_blocks(need_block_num);
            wit = db.get_witness_by_uid(u_1000_id);
            const dynamic_global_property_object& dpo = db.get_dynamic_global_properties();

            share_type pledge_bonus = ((fc::bigint)dpo.by_pledge_witness_pay_per_block.value * wit.bonus_rate * wit.total_mining_pledge
               / ((wit.pledge + wit.total_mining_pledge) * GRAPHENE_100_PERCENT)).to_int64();
            auto produce_blocks_per_cycle = wit.total_produced - last_produce_blocks;
            last_produce_blocks = wit.total_produced;

            share_type bonus_per_pledge = 0;
            if (i == 0 && is_produce_block)
            {
               share_type last_bonus = ((fc::bigint)last_pledge_witness_pay.value * wit.bonus_rate * wit.total_mining_pledge
                  / ((wit.pledge + wit.total_mining_pledge) * GRAPHENE_100_PERCENT)).to_int64();

               bonus_per_pledge = ((fc::uint128_t)((produce_blocks_per_cycle - 1) * pledge_bonus + last_bonus).value * GRAPHENE_PLEDGE_BONUS_PRECISION
                  / wit.total_mining_pledge).to_uint64();
            }
            else
               bonus_per_pledge = ((fc::uint128_t)(produce_blocks_per_cycle * pledge_bonus).value * GRAPHENE_PLEDGE_BONUS_PRECISION
               / wit.total_mining_pledge).to_uint64();

            int j = 0;
            for (const auto& m : mining_map)
            {
               auto mining_obj = db.get_pledge_mining(u_1000_id, u_3001_id);
               share_type bonus = ((fc::uint128_t)bonus_per_pledge.value * mining_obj.pledge_id(db).pledge.value
                  / GRAPHENE_PLEDGE_BONUS_PRECISION).to_uint64();
               total_bonus2 += bonus;
               total_bonus_per_account.at(j) += bonus;
               auto account = db.get_account_statistics_by_uid(m.first);
               BOOST_CHECK(account.uncollected_pledge_bonus == total_bonus_per_account.at(j));
               ++j;
               bonus_3001 = bonus;
            }
            total_bonus2 += bonus_3001;

            uint32_t block_num = wit.total_produced - produce_block;
            if (is_produce_block)
            {
               witness_pay2 = dpo.by_pledge_witness_pay_per_block*(block_num - 1) + last_pledge_witness_pay - total_bonus2;
            }
            else
               witness_pay2 = dpo.by_pledge_witness_pay_per_block*block_num - total_bonus2;
            auto witness = db.get_account_statistics_by_uid(u_1000_id);
            auto uncollected_witness_pay = witness_pay + witness_pay2;
            BOOST_CHECK(witness.uncollected_witness_pay == uncollected_witness_pay);
         }

         last_produce_blocks = db.get_witness_by_uid(u_1000_id).total_produced;
         auto witness_uncollet_pay = db.get_account_statistics_by_uid(u_1000_id).uncollected_witness_pay;

         auto last_account2_3001_bonus = db.get_account_statistics_by_uid(u_3001_id).uncollected_pledge_bonus;
         generate_blocks(5000);

         //cancel mining pledge
         update_mining_pledge({ u_3001_private_key }, u_3001_id, u_1000_id, 0);
         auto wit3 = db.get_witness_by_uid(u_1000_id);
         const dynamic_global_property_object& dpo3 = db.get_dynamic_global_properties();
         share_type pledge_bonus3 = ((fc::bigint)dpo3.by_pledge_witness_pay_per_block.value * wit3.bonus_rate * (wit3.total_mining_pledge + 200000 * prec.value)
            / ((wit3.pledge + wit3.total_mining_pledge + 200000 * prec.value) * GRAPHENE_100_PERCENT)).to_int64();
         share_type bonus_per_pledge3 = ((fc::uint128_t)((wit3.total_produced - last_produce_blocks) * pledge_bonus3).value * GRAPHENE_PLEDGE_BONUS_PRECISION
            / wit.total_mining_pledge).to_uint64();
         share_type bonus3_3001 = ((fc::uint128_t)bonus_per_pledge3.value * 200000 * prec.value
            / GRAPHENE_PLEDGE_BONUS_PRECISION).to_uint64();
         auto account3_3001 = db.get_account_statistics_by_uid(u_3001_id);
         share_type uncollected_pledge_bonus_3001 = bonus3_3001 + last_account2_3001_bonus;
         BOOST_CHECK(account3_3001.uncollected_pledge_bonus == uncollected_pledge_bonus_3001);

         int k = 0;
         for (const auto&p : mining_map)
         {
            //auto mining_obj = db.get_pledge_mining(u_1000_id, u_3001_id);
            update_mining_pledge({ p.second }, p.first, u_1000_id, 0);
            auto account3 = db.get_account_statistics_by_uid(p.first);
            BOOST_CHECK(account3.uncollected_pledge_bonus == bonus3_3001 + total_bonus_per_account.at(k));
            ++k;
         }

         auto total_bonus_3 = bonus3_3001 * (mining_map.size() + 1);
         auto uncollet_witness_pay3 = dpo3.by_pledge_witness_pay_per_block.value * (wit3.total_produced - last_produce_blocks) - total_bonus_3;
         auto wit3_pay = db.get_account_statistics_by_uid(u_1000_id).uncollected_witness_pay;
         BOOST_CHECK(wit3_pay == uncollet_witness_pay3 + witness_uncollet_pay);

         generate_blocks(6000);
         auto wit4 = db.get_witness_by_uid(u_1000_id);
         BOOST_CHECK(wit4.total_mining_pledge == 0);
         BOOST_CHECK(wit4.bonus_per_pledge.size() == 0);
         BOOST_CHECK(wit4.unhandled_bonus == 0);
         BOOST_CHECK(wit4.need_distribute_bonus == 0);
         BOOST_CHECK(wit4.already_distribute_bonus == 0);

         //test release_mining_pledge
         generate_blocks(28800 * 7);

         for (const auto&p : mining_map)
         {
            auto mining_obj = db.find_pledge_mining(u_1000_id, u_3001_id);
            BOOST_CHECK(mining_obj == nullptr);
         }
         verify_asset_supplies(db);
   }
   catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(pledge_mining_test_2)
{
   try{
         ACTORS((1000)(2000)(3001)(4001));

         const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
         auto _core = [&](int64_t x) -> asset
         {  return asset(x*prec);    };
         transfer(committee_account, u_1000_id, _core(600000));
         transfer(committee_account, u_2000_id, _core(600000));

         transfer(committee_account, u_3001_id, _core(600000));

         add_csaf_for_account(u_1000_id, 10000);
         add_csaf_for_account(u_2000_id, 10000);
         generate_blocks(HARDFORK_0_5_TIME, true);

         graphene::chain::pledge_mining::ext ext;
         ext.can_pledge = true;
         ext.bonus_rate = 6000;
         create_witness({ u_1000_private_key }, u_1000_id, "test pledge witness-1", 500000 * prec, u_1000_public_key, ext);
         create_witness({ u_2000_private_key }, u_2000_id, "test pledge witness-2", 500000 * prec, u_2000_public_key, ext);

         update_mining_pledge({ u_3001_private_key }, u_3001_id, u_1000_id, 200000 * prec.value);
         update_mining_pledge({ u_3001_private_key }, u_3001_id, u_2000_id, 100000 * prec.value);

         generate_blocks(9000);

         update_mining_pledge({ u_3001_private_key }, u_3001_id, u_1000_id, 0);
         update_mining_pledge({ u_3001_private_key }, u_3001_id, u_2000_id, 0);

         auto wit1 = db.get_witness_by_uid(u_1000_id);
         auto wit2 = db.get_witness_by_uid(u_2000_id);

         const dynamic_global_property_object& dpo = db.get_dynamic_global_properties();
         share_type pledge_bonus1 = ((fc::bigint)(dpo.by_pledge_witness_pay_per_block.value) * 6000 * 200000 * prec.value
            / (700000 * prec.value * GRAPHENE_100_PERCENT)).to_int64();
         share_type pledge_bonus2 = ((fc::bigint)(dpo.by_pledge_witness_pay_per_block.value) * 6000 * 100000 * prec.value
            / (600000 * prec.value * GRAPHENE_100_PERCENT)).to_int64();

         auto account_3001 = db.get_account_statistics_by_uid(u_3001_id);
         auto bonus1 = wit1.total_produced * pledge_bonus1;
         auto bonus2 = wit2.total_produced * pledge_bonus2;
         BOOST_CHECK(account_3001.uncollected_pledge_bonus == bonus1 + bonus2);

         auto witness_act1 = db.get_account_statistics_by_uid(u_1000_id);
         auto witness_act2 = db.get_account_statistics_by_uid(u_2000_id);
         auto wit_pay1 = dpo.by_pledge_witness_pay_per_block.value * wit1.total_produced - bonus1;
         auto wit_pay2 = dpo.by_pledge_witness_pay_per_block.value * wit2.total_produced - bonus2;
         BOOST_CHECK(witness_act1.uncollected_witness_pay == wit_pay1);
         BOOST_CHECK(witness_act2.uncollected_witness_pay == wit_pay2);
         
   }
   catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(pledge_mining_test_3)
{
   try{
      ACTORS((1000)(2000)(3001));

      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };
      transfer(committee_account, u_1000_id, _core(600000));
      transfer(committee_account, u_2000_id, _core(600000));

      transfer(committee_account, u_3001_id, _core(600000));

      add_csaf_for_account(u_1000_id, 10000);
      add_csaf_for_account(u_2000_id, 10000);
      generate_blocks(HARDFORK_0_5_TIME, true);

      graphene::chain::pledge_mining::ext ext;
      ext.can_pledge = true;
      ext.bonus_rate = 6000;
      create_witness({ u_1000_private_key }, u_1000_id, "test pledge witness-1", 500000 * prec, u_1000_public_key, ext);
      update_mining_pledge({ u_3001_private_key }, u_3001_id, u_1000_id, 200000 * prec.value);
      generate_blocks(5000);

      update_mining_pledge({ u_3001_private_key }, u_3001_id, u_1000_id, 100000 * prec.value);

      const dynamic_global_property_object& dpo = db.get_dynamic_global_properties();
      auto last_witness_pay = dpo.by_pledge_witness_pay_per_block;
      share_type pledge_bonus1 = ((fc::bigint)(dpo.by_pledge_witness_pay_per_block.value) * 6000 * 200000 * prec.value
         / (700000 * prec.value * GRAPHENE_100_PERCENT)).to_int64();
      auto wit1 = db.get_witness_by_uid(u_1000_id);
      auto bonus1 = wit1.total_produced * pledge_bonus1;
      auto account1 = db.get_account_statistics_by_uid(u_3001_id);
      BOOST_CHECK(account1.uncollected_pledge_bonus == bonus1);

      auto wit_pay1 = dpo.by_pledge_witness_pay_per_block.value * wit1.total_produced - bonus1;
      auto witness_act1 = db.get_account_statistics_by_uid(u_1000_id);
      BOOST_CHECK(witness_act1.uncollected_witness_pay == wit_pay1);

      auto last_pledge_witness_pay = dpo.by_pledge_witness_pay_per_block;
      auto last_produce_block = wit1.total_produced;
      bool is_produce_block = false;
      for (int i = 0; i < 20; ++i)
      {
         generate_block();
         const dynamic_global_property_object& dpo = db.get_dynamic_global_properties();
         if (last_pledge_witness_pay != dpo.by_pledge_witness_pay_per_block)
         {
            auto witness = db.get_witness_by_uid(u_1000_id);
            if (last_produce_block != db.get_witness_by_uid(u_1000_id).total_produced)
               is_produce_block = true;
            break;
         }
      }

      auto block_num = db.get_witness_by_uid(u_1000_id).last_update_bonus_block_num + 10000 - db.head_block_num();
      generate_blocks(block_num);

      auto wit2 = db.get_witness_by_uid(u_1000_id);
      const dynamic_global_property_object& dpo2 = db.get_dynamic_global_properties();
      share_type pledge_bonus2 = ((fc::bigint)(dpo2.by_pledge_witness_pay_per_block.value) * 6000 * 100000 * prec.value
         / (600000 * prec.value * GRAPHENE_100_PERCENT)).to_int64();
      share_type bonus_per_pledge = 0;
      if (is_produce_block)
      {
         share_type bonus = ((fc::bigint)(last_witness_pay.value) * 6000 * 100000 * prec.value
            / (600000 * prec.value * GRAPHENE_100_PERCENT)).to_int64();
         auto total_bonus = (wit2.total_produced - last_produce_block - 1) * pledge_bonus2 + bonus;
         bonus_per_pledge = ((fc::uint128_t)total_bonus.value * GRAPHENE_PLEDGE_BONUS_PRECISION
            / wit2.total_mining_pledge).to_uint64();
      }
      else
         bonus_per_pledge = ((fc::uint128_t)((wit2.total_produced - last_produce_block) * pledge_bonus2).value * GRAPHENE_PLEDGE_BONUS_PRECISION
         / wit2.total_mining_pledge).to_uint64();

      share_type bonus2 = ((fc::uint128_t)bonus_per_pledge.value * 100000 * prec.value
         / GRAPHENE_PLEDGE_BONUS_PRECISION).to_uint64();

      auto account2 = db.get_account_statistics_by_uid(u_3001_id);
      auto account_uncollect_bonus = bonus1 + bonus2;
      BOOST_CHECK(account2.uncollected_pledge_bonus == account_uncollect_bonus);
   }
   catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}


//check check pledge mining, after resign witness
BOOST_AUTO_TEST_CASE(pledge_mining_test_4)
{
   try{
      ACTORS((1000)(2000)(3001));

      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };
      transfer(committee_account, u_1000_id, _core(600000));
      transfer(committee_account, u_2000_id, _core(600000));

      transfer(committee_account, u_3001_id, _core(600000));

      add_csaf_for_account(u_1000_id, 10000);
      add_csaf_for_account(u_2000_id, 10000);
      generate_blocks(HARDFORK_0_5_TIME, true);

      graphene::chain::pledge_mining::ext ext;
      ext.can_pledge = true;
      ext.bonus_rate = 6000;
      create_witness({ u_1000_private_key }, u_1000_id, "test pledge witness-1", 500000 * prec, u_1000_public_key, ext);
      create_witness({ u_2000_private_key }, u_2000_id, "test pledge witness-2", 500000 * prec, u_2000_public_key, ext);

      update_mining_pledge({ u_3001_private_key }, u_3001_id, u_1000_id, 200000 * prec.value);

      for (;;)
      {
         generate_blocks(11);
         const auto& active_witnesses = db.get_global_properties().active_witnesses;
         if (active_witnesses.count(u_1000_id) == 0) {
            update_witness({ u_1000_private_key }, u_1000_id, optional<public_key_type>(), _core(0), optional<string>());
            break;
         }
      }
      //check pledge mining, after resign witness
      generate_blocks(10);
      const pledge_mining_object& pledge_mining_obj = db.get_pledge_mining(u_1000_id, u_3001_id);
      const auto& pledge_balnce_obj = pledge_mining_obj.pledge_id(db);
      BOOST_CHECK(pledge_balnce_obj.pledge == 0);
      BOOST_CHECK(pledge_balnce_obj.releasing_pledges.size() == 1);
      BOOST_CHECK(pledge_balnce_obj.releasing_pledges.begin()->first == db.head_block_num() - 9 + GRAPHENE_DEFAULT_MINING_PLEDGE_RELEASE_DELAY);
      BOOST_CHECK(pledge_balnce_obj.releasing_pledges.begin()->second == 200000 * prec);

      auto pledge_act_3001 = db.get_account_statistics_by_uid(u_3001_id);
      BOOST_CHECK(pledge_act_3001.total_mining_pledge == 200000 * prec);

      update_mining_pledge({ u_3001_private_key }, u_3001_id, u_2000_id, 300000 * prec.value);

      generate_blocks(28800 * 7);
      const pledge_mining_object* pledge_mining_obj_ptr = db.find_pledge_mining(u_1000_id, u_3001_id);
      BOOST_CHECK(pledge_mining_obj_ptr == nullptr);

      const pledge_mining_object* pledge_mining_obj_ptr2 = db.find_pledge_mining(u_2000_id, u_3001_id);
      const auto& pledge_balnce_obj2 = pledge_mining_obj_ptr2->pledge_id(db);
      BOOST_CHECK(pledge_balnce_obj2.releasing_pledges.size() == 0);
      BOOST_CHECK(pledge_balnce_obj2.pledge == 300000 * prec.value);

      auto pledge_act_3001_2 = db.get_account_statistics_by_uid(u_3001_id);
      BOOST_CHECK(pledge_act_3001_2.total_mining_pledge == 300000 * prec.value);

   }
   catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

//check witness after refactoring
BOOST_AUTO_TEST_CASE(witness_test)
{
   try{
      ACTORS((1000)(2000));

      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };
      transfer(committee_account, u_1000_id, _core(1000000));
      transfer(committee_account, u_2000_id, _core(1000000));

      add_csaf_for_account(u_1000_id, 10000);
      add_csaf_for_account(u_2000_id, 10000);

      graphene::chain::pledge_mining::ext ext;
      ext.can_pledge = true;
      ext.bonus_rate = 6000;

      // check  hard fork 04
      //*************************************
      create_witness({ u_1000_private_key }, u_1000_id, "test pledge witness-1", 80000 * prec, u_1000_public_key, ext);
      update_witness({ u_1000_private_key }, u_1000_id, optional<public_key_type>(), _core(50000), optional<string>());
      generate_blocks(1);
      const auto& user1 = db.get_account_statistics_by_uid(u_1000_id);
      const auto& pledge_balance_obj1 = user1.pledge_balance_ids.at(pledge_balance_type::Witness)(db);
      BOOST_CHECK(pledge_balance_obj1.pledge == 50000 * prec);
      BOOST_CHECK(pledge_balance_obj1.total_releasing_pledge == 30000 * prec);
      BOOST_CHECK(pledge_balance_obj1.releasing_pledges.size() == 1);
      BOOST_CHECK(pledge_balance_obj1.releasing_pledges.begin()->first == db.head_block_num() + GRAPHENE_DEFAULT_COMMITTEE_MEMBER_PLEDGE_RELEASE_DELAY);

      //check hardfork 04, release size is 1
      update_witness({ u_1000_private_key }, u_1000_id, optional<public_key_type>(), _core(30000), optional<string>());
      generate_blocks(1);
      const auto& user1_1 = db.get_account_statistics_by_uid(u_1000_id);
      const auto& pledge_balance_obj1_1 = user1_1.pledge_balance_ids.at(pledge_balance_type::Witness)(db);
      BOOST_CHECK(pledge_balance_obj1_1.pledge == 30000 * prec);
      BOOST_CHECK(pledge_balance_obj1_1.total_releasing_pledge == 50000 * prec);
      BOOST_CHECK(pledge_balance_obj1_1.releasing_pledges.size() == 1);
      BOOST_CHECK(pledge_balance_obj1_1.releasing_pledges.begin()->first == db.head_block_num() + GRAPHENE_DEFAULT_COMMITTEE_MEMBER_PLEDGE_RELEASE_DELAY);
      //check clear release size
      generate_blocks(GRAPHENE_DEFAULT_WITNESS_PLEDGE_RELEASE_DELAY);
      const auto& user1_2 = db.get_account_statistics_by_uid(u_1000_id);
      const auto& pledge_balance_obj1_2 = user1_2.pledge_balance_ids.at(pledge_balance_type::Witness)(db);
      BOOST_CHECK(pledge_balance_obj1_2.pledge == 30000 * prec);
      BOOST_CHECK(pledge_balance_obj1_2.releasing_pledges.size() == 0);
      BOOST_CHECK(pledge_balance_obj1_2.total_releasing_pledge == 0);


      generate_blocks(HARDFORK_0_5_TIME, true);

      //*************************************
      // check  hard fork 05
      //*************************************

      create_witness({ u_2000_private_key }, u_2000_id, "test pledge witness-2", 50000 * prec, u_2000_public_key, ext);
      //test create committee
      const auto& user2 = db.get_account_statistics_by_uid(u_2000_id);
      const auto& pledge_balance_obj2 = user2.pledge_balance_ids.at(pledge_balance_type::Witness)(db);
      BOOST_CHECK(pledge_balance_obj2.pledge == 50000 * prec);
      BOOST_CHECK(pledge_balance_obj2.total_releasing_pledge == 0 * prec);
      BOOST_CHECK(pledge_balance_obj2.releasing_pledges.size() == 0 * prec);

      //test update committee
      update_witness({ u_2000_private_key }, u_2000_id, optional<public_key_type>(), _core(30000), optional<string>());
      generate_blocks(1);
      const auto& user4 = db.get_account_statistics_by_uid(u_2000_id);
      const auto& pledge_balance_obj4 = user4.pledge_balance_ids.at(pledge_balance_type::Witness)(db);
      BOOST_CHECK(pledge_balance_obj4.pledge == 30000 * prec);
      BOOST_CHECK(pledge_balance_obj4.total_releasing_pledge == 20000 * prec);
      BOOST_CHECK(pledge_balance_obj4.releasing_pledges.size() == 1);
      BOOST_CHECK(pledge_balance_obj4.releasing_pledges.begin()->first == db.head_block_num() + GRAPHENE_DEFAULT_WITNESS_PLEDGE_RELEASE_DELAY);
      BOOST_CHECK(pledge_balance_obj4.releasing_pledges.begin()->second == 20000 * prec);

      //test new releasing
      update_witness({ u_2000_private_key }, u_2000_id, optional<public_key_type>(), _core(20000), optional<string>());
      generate_blocks(1);
      const auto& user5 = db.get_account_statistics_by_uid(u_2000_id);
      const auto& pledge_balance_obj5 = user5.pledge_balance_ids.at(pledge_balance_type::Witness)(db);
      BOOST_CHECK(pledge_balance_obj5.pledge == 20000 * prec);
      BOOST_CHECK(pledge_balance_obj5.total_releasing_pledge == 30000 * prec);
      BOOST_CHECK(pledge_balance_obj5.releasing_pledges.size() == 2);
      BOOST_CHECK(pledge_balance_obj5.releasing_pledges.rbegin()->first == db.head_block_num() + GRAPHENE_DEFAULT_WITNESS_PLEDGE_RELEASE_DELAY);
      BOOST_CHECK(pledge_balance_obj5.releasing_pledges.rbegin()->second == 10000 * prec);

      //test reduce releasing
      update_witness({ u_2000_private_key }, u_2000_id, optional<public_key_type>(), _core(25000), optional<string>());
      generate_blocks(1);
      const auto& user6 = db.get_account_statistics_by_uid(u_2000_id);
      const auto& pledge_balance_obj6 = user6.pledge_balance_ids.at(pledge_balance_type::Witness)(db);
      BOOST_CHECK(pledge_balance_obj6.pledge == 25000 * prec);
      BOOST_CHECK(pledge_balance_obj6.total_releasing_pledge == 25000 * prec);
      BOOST_CHECK(pledge_balance_obj6.releasing_pledges.size() == 2);
      BOOST_CHECK(pledge_balance_obj6.releasing_pledges.rbegin()->first == db.head_block_num() - 1 + GRAPHENE_DEFAULT_WITNESS_PLEDGE_RELEASE_DELAY);
      BOOST_CHECK(pledge_balance_obj6.releasing_pledges.rbegin()->second == 5000 * prec);

      generate_blocks(GRAPHENE_DEFAULT_WITNESS_PLEDGE_RELEASE_DELAY);
      const auto& user7 = db.get_account_statistics_by_uid(u_2000_id);
      const auto& pledge_balance_obj7 = user7.pledge_balance_ids.at(pledge_balance_type::Witness)(db);
      BOOST_CHECK(pledge_balance_obj7.pledge == 25000 * prec);
      BOOST_CHECK(pledge_balance_obj7.releasing_pledges.size() == 0);
      BOOST_CHECK(pledge_balance_obj7.total_releasing_pledge == 0);

   }
   catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

//check committee after refactoring
BOOST_AUTO_TEST_CASE(committee_test)
{
   try{
      ACTORS((1000)(2000));

      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };
      transfer(committee_account, u_1000_id, _core(600000));
      transfer(committee_account, u_2000_id, _core(600000));

      add_csaf_for_account(u_1000_id, 10000);
      add_csaf_for_account(u_2000_id, 10000);


      //*************************************
      // check  hard fork 04
      //*************************************
      create_committee({ u_1000_private_key }, u_1000_id, "committee test1", 8000 * prec.value);
      update_committee({ u_1000_private_key }, u_1000_id, 5000 * prec.value);
      generate_blocks(1);
      const auto& user1 = db.get_account_statistics_by_uid(u_1000_id);
      const auto& pledge_balance_obj1 = user1.pledge_balance_ids.at(pledge_balance_type::Commitment)(db);
      BOOST_CHECK(pledge_balance_obj1.pledge == 5000 * prec);
      BOOST_CHECK(pledge_balance_obj1.total_releasing_pledge == 3000 * prec);
      BOOST_CHECK(pledge_balance_obj1.releasing_pledges.size() == 1);
      BOOST_CHECK(pledge_balance_obj1.releasing_pledges.begin()->first == db.head_block_num() + GRAPHENE_DEFAULT_COMMITTEE_MEMBER_PLEDGE_RELEASE_DELAY);

      //check hardfork 04, release size is 1
      update_committee({ u_1000_private_key }, u_1000_id, 3000 * prec.value);
      generate_blocks(1);
      const auto& user1_1 = db.get_account_statistics_by_uid(u_1000_id);
      const auto& pledge_balance_obj1_1 = user1_1.pledge_balance_ids.at(pledge_balance_type::Commitment)(db);
      BOOST_CHECK(pledge_balance_obj1_1.pledge == 3000 * prec);
      BOOST_CHECK(pledge_balance_obj1_1.total_releasing_pledge == 5000 * prec);
      BOOST_CHECK(pledge_balance_obj1_1.releasing_pledges.size() == 1);
      BOOST_CHECK(pledge_balance_obj1_1.releasing_pledges.begin()->first == db.head_block_num() + GRAPHENE_DEFAULT_COMMITTEE_MEMBER_PLEDGE_RELEASE_DELAY);
      //check clear release size
      generate_blocks(GRAPHENE_DEFAULT_COMMITTEE_MEMBER_PLEDGE_RELEASE_DELAY);
      const auto& user1_2 = db.get_account_statistics_by_uid(u_1000_id);
      const auto& pledge_balance_obj1_2 = user1_2.pledge_balance_ids.at(pledge_balance_type::Commitment)(db);
      BOOST_CHECK(pledge_balance_obj1_2.pledge == 3000 * prec);
      BOOST_CHECK(pledge_balance_obj1_2.releasing_pledges.size() == 0);
      BOOST_CHECK(pledge_balance_obj1_2.total_releasing_pledge == 0);


      generate_blocks(HARDFORK_0_5_TIME, true);

      //*************************************
      // check  hard fork 05
      //*************************************

      create_committee({ u_2000_private_key }, u_2000_id, "committee test1", 5000 * prec.value);
      //test create committee
      const auto& user2 = db.get_account_statistics_by_uid(u_2000_id);
      const auto& pledge_balance_obj2 = user2.pledge_balance_ids.at(pledge_balance_type::Commitment)(db);
      BOOST_CHECK(pledge_balance_obj2.pledge == 5000 * prec);
      BOOST_CHECK(pledge_balance_obj2.total_releasing_pledge == 0 * prec);
      BOOST_CHECK(pledge_balance_obj2.releasing_pledges.size() == 0 * prec);

      //test update committee
      update_committee({ u_2000_private_key }, u_2000_id, 3000 * prec.value);
      generate_blocks(1);
      const auto& user4 = db.get_account_statistics_by_uid(u_2000_id);
      const auto& pledge_balance_obj4 = user4.pledge_balance_ids.at(pledge_balance_type::Commitment)(db);
      BOOST_CHECK(pledge_balance_obj4.pledge == 3000 * prec);
      BOOST_CHECK(pledge_balance_obj4.total_releasing_pledge == 2000 * prec);
      BOOST_CHECK(pledge_balance_obj4.releasing_pledges.size() == 1);
      BOOST_CHECK(pledge_balance_obj4.releasing_pledges.begin()->first == db.head_block_num() + GRAPHENE_DEFAULT_COMMITTEE_MEMBER_PLEDGE_RELEASE_DELAY);
      BOOST_CHECK(pledge_balance_obj4.releasing_pledges.begin()->second == 2000 * prec);

      //test new releasing
      update_committee({ u_2000_private_key }, u_2000_id, 2000 * prec);
      generate_blocks(1);
      const auto& user5 = db.get_account_statistics_by_uid(u_2000_id);
      const auto& pledge_balance_obj5 = user5.pledge_balance_ids.at(pledge_balance_type::Commitment)(db);
      BOOST_CHECK(pledge_balance_obj5.pledge == 2000 * prec);
      BOOST_CHECK(pledge_balance_obj5.total_releasing_pledge == 3000 * prec);
      BOOST_CHECK(pledge_balance_obj5.releasing_pledges.size() == 2);
      BOOST_CHECK(pledge_balance_obj5.releasing_pledges.rbegin()->first == db.head_block_num() + GRAPHENE_DEFAULT_COMMITTEE_MEMBER_PLEDGE_RELEASE_DELAY);
      BOOST_CHECK(pledge_balance_obj5.releasing_pledges.rbegin()->second == 1000 * prec);

      //test reduce releasing
      update_committee({ u_2000_private_key }, u_2000_id, 2500 * prec);
      generate_blocks(1);
      const auto& user6 = db.get_account_statistics_by_uid(u_2000_id);
      const auto& pledge_balance_obj6 = user6.pledge_balance_ids.at(pledge_balance_type::Commitment)(db);
      BOOST_CHECK(pledge_balance_obj6.pledge == 2500 * prec);
      BOOST_CHECK(pledge_balance_obj6.total_releasing_pledge == 2500 * prec);
      BOOST_CHECK(pledge_balance_obj6.releasing_pledges.size() == 2);
      BOOST_CHECK(pledge_balance_obj6.releasing_pledges.rbegin()->first == db.head_block_num() - 1 + GRAPHENE_DEFAULT_COMMITTEE_MEMBER_PLEDGE_RELEASE_DELAY);
      BOOST_CHECK(pledge_balance_obj6.releasing_pledges.rbegin()->second == 500 * prec);

      generate_blocks(GRAPHENE_DEFAULT_COMMITTEE_MEMBER_PLEDGE_RELEASE_DELAY);
      const auto& user7 = db.get_account_statistics_by_uid(u_2000_id);
      const auto& pledge_balance_obj7 = user7.pledge_balance_ids.at(pledge_balance_type::Commitment)(db);
      BOOST_CHECK(pledge_balance_obj7.pledge == 2500 * prec);
      BOOST_CHECK(pledge_balance_obj7.releasing_pledges.size() == 0);
      BOOST_CHECK(pledge_balance_obj7.total_releasing_pledge == 0);

   }
   catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE(destory_asset_test)
{
   try
   {
      ACTORS((1000));
      const share_type prec = asset::scaled_precision(asset_id_type()(db).precision);
      auto _core = [&](int64_t x) -> asset
      {  return asset(x*prec);    };

      transfer(committee_account, u_1000_id, _core(100000000));
      collect_csaf_from_committee(u_1000_id, 100);
      generate_blocks(HARDFORK_0_5_TIME, true);

      share_type destory_amount = 100000000;
      share_type origin_amount = db.get_balance(u_1000_id, GRAPHENE_CORE_ASSET_AID).amount;
      share_type origin_supply = db.get_asset_by_aid(GRAPHENE_CORE_ASSET_AID).dynamic_data(db).current_supply;

      destroy_asset({ u_1000_private_key }, u_1000_id, GRAPHENE_CORE_ASSET_AID, 100000000);
      share_type now_amount = db.get_balance(u_1000_id, GRAPHENE_CORE_ASSET_AID).amount;
      share_type now_supply = db.get_asset_by_aid(GRAPHENE_CORE_ASSET_AID).dynamic_data(db).current_supply;

      BOOST_CHECK(now_amount == (origin_amount - destory_amount));
      BOOST_CHECK(now_supply == (origin_supply - destory_amount));

   }
   catch (const fc::exception& e)
   {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_SUITE_END()
