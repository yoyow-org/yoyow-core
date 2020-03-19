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
#pragma once

#include <graphene/app/application.hpp>
#include <graphene/chain/database.hpp>
#include <fc/io/json.hpp>
#include <fc/smart_ref_impl.hpp>

#include <graphene/chain/operation_history_object.hpp>
#include <boost/multiprecision/cpp_int.hpp>

#include <iostream>

using namespace graphene::db;

extern uint32_t GRAPHENE_TESTING_GENESIS_TIMESTAMP;

typedef boost::multiprecision::uint128_t uint128_t;

#define PUSH_TX \
   graphene::chain::test::_push_transaction

#define PUSH_BLOCK \
   graphene::chain::test::_push_block

// See below
#define REQUIRE_OP_VALIDATION_SUCCESS( op, field, value ) \
{ \
   const auto temp = op.field; \
   op.field = value; \
   op.validate(); \
   op.field = temp; \
}
#define REQUIRE_OP_EVALUATION_SUCCESS( op, field, value ) \
{ \
   const auto temp = op.field; \
   op.field = value; \
   trx.operations.back() = op; \
   op.field = temp; \
   db.push_transaction( trx, ~0 ); \
}

#define GRAPHENE_REQUIRE_THROW( expr, exc_type )          \
{                                                         \
   std::string req_throw_info = fc::json::to_string(      \
      fc::mutable_variant_object()                        \
      ("source_file", __FILE__)                           \
      ("source_lineno", __LINE__)                         \
      ("expr", #expr)                                     \
      ("exc_type", #exc_type)                             \
      );                                                  \
   if( fc::enable_record_assert_trip )                    \
      std::cout << "GRAPHENE_REQUIRE_THROW begin "        \
         << req_throw_info << std::endl;                  \
   BOOST_REQUIRE_THROW( expr, exc_type );                 \
   if( fc::enable_record_assert_trip )                    \
      std::cout << "GRAPHENE_REQUIRE_THROW end "          \
         << req_throw_info << std::endl;                  \
}

#define GRAPHENE_CHECK_THROW( expr, exc_type )            \
{                                                         \
   std::string req_throw_info = fc::json::to_string(      \
      fc::mutable_variant_object()                        \
      ("source_file", __FILE__)                           \
      ("source_lineno", __LINE__)                         \
      ("expr", #expr)                                     \
      ("exc_type", #exc_type)                             \
      );                                                  \
   if( fc::enable_record_assert_trip )                    \
      std::cout << "GRAPHENE_CHECK_THROW begin "          \
         << req_throw_info << std::endl;                  \
   BOOST_CHECK_THROW( expr, exc_type );                   \
   if( fc::enable_record_assert_trip )                    \
      std::cout << "GRAPHENE_CHECK_THROW end "            \
         << req_throw_info << std::endl;                  \
}

#define REQUIRE_OP_VALIDATION_FAILURE_2( op, field, value, exc_type ) \
{ \
   const auto temp = op.field; \
   op.field = value; \
   GRAPHENE_REQUIRE_THROW( op.validate(), exc_type ); \
   op.field = temp; \
}
#define REQUIRE_OP_VALIDATION_FAILURE( op, field, value ) \
   REQUIRE_OP_VALIDATION_FAILURE_2( op, field, value, fc::exception )

#define REQUIRE_THROW_WITH_VALUE_2(op, field, value, exc_type) \
{ \
   auto bak = op.field; \
   op.field = value; \
   trx.operations.back() = op; \
   op.field = bak; \
   GRAPHENE_REQUIRE_THROW(db.push_transaction(trx, ~0), exc_type); \
}

#define REQUIRE_THROW_WITH_VALUE( op, field, value ) \
   REQUIRE_THROW_WITH_VALUE_2( op, field, value, fc::exception )

///This simply resets v back to its default-constructed value. Requires v to have a working assingment operator and
/// default constructor.
#define RESET(v) v = decltype(v)()
///This allows me to build consecutive test cases. It's pretty ugly, but it works well enough for unit tests.
/// i.e. This allows a test on update_account to begin with the database at the end state of create_account.
#define INVOKE(test) ((struct test*)this)->test_method(); trx.clear()

#define PREP_ACTOR(seed) \
   fc::ecc::private_key u_ ## seed ## _private_key = generate_private_key(BOOST_PP_STRINGIZE(seed));   \
   public_key_type u_ ## seed ## _public_key = u_ ## seed ## _private_key.get_public_key();

#define ACTOR(seed) \
   PREP_ACTOR(seed) \
   account_uid_type u_ ## seed ## _id = graphene::chain::calc_account_uid(seed); \
   const auto& u_ ## seed = create_account(u_ ## seed ## _id, BOOST_PP_STRINGIZE(u##seed), u_ ## seed ## _public_key); \

#define GET_ACTOR(seed) \
   fc::ecc::private_key u_ ## seed ## _private_key = generate_private_key(BOOST_PP_STRINGIZE(seed)); \
   const account_object& u_ ## seed = get_account(BOOST_PP_STRINGIZE(u##seed)); \
   account_uid_type u_ ## seed ## _id = u_ ## seed.uid; \

#define ACTORS_IMPL(r, data, elem) ACTOR(elem)
#define ACTORS(seeds) BOOST_PP_SEQ_FOR_EACH(ACTORS_IMPL, ~, seeds)

namespace graphene { namespace chain {

struct database_fixture {
   // the reason we use an app is to exercise the indexes of built-in
   //   plugins
   graphene::app::application app;
   genesis_state_type genesis_state;
   chain::database &db;
   signed_transaction trx;
   public_key_type committee_key;
   account_uid_type committee_account = GRAPHENE_COMMITTEE_ACCOUNT_UID;
   fc::ecc::private_key private_key = fc::ecc::private_key::generate();
   fc::ecc::private_key init_account_priv_key = fc::ecc::private_key::regenerate(fc::sha256::hash(string("null_key")));
   public_key_type init_account_pub_key;

   optional<fc::temp_directory> data_dir;
   bool skip_key_index_test = false;
   uint32_t anon_acct_count;

   database_fixture();
   ~database_fixture();

   static fc::ecc::private_key generate_private_key(string seed);
   string generate_anon_acct_name();
   static void verify_asset_supplies(const database& db);
   void verify_account_history_plugin_index()const;
   void open_database();
   signed_block generate_block(uint32_t skip = ~0,
      const fc::ecc::private_key& key = generate_private_key("null_key"),
      int miss_blocks = 0);

   /**
    * @brief Generates block_count blocks
    * @param block_count number of blocks to generate
    */
   void generate_blocks(uint32_t block_count);

   /**
    * @brief Generates blocks until the head block time matches or exceeds timestamp
    * @param timestamp target time to generate blocks until
    */
   void generate_blocks(fc::time_point_sec timestamp, bool miss_intermediate_blocks = true, uint32_t skip = ~0);

   account_create_operation make_account_seed(
      uint32_t seed = 100,
      const std::string& name = "nathan",
      public_key_type = public_key_type()
      );

   account_create_operation make_account(
      account_uid_type uid,
      const std::string& name = "nathan",
      public_key_type = public_key_type()
      );

   account_create_operation make_account(
      account_uid_type uid,
      const std::string& name,
      const account_object& registrar,
      const account_object& referrer,
      uint8_t referrer_percent = 100,
      public_key_type key = public_key_type()
      );

   const asset_object& get_asset(const string& symbol)const;
   const account_object& get_account(const string& name)const;

   const asset_object& create_user_issued_asset(const string& name);
   const asset_object& create_user_issued_asset(const string& name,
      const account_object& issuer,
      uint16_t flags);
   void issue_uia(const account_object& recipient, asset amount);
   void issue_uia(account_uid_type recipient_id, asset amount);

   const account_object& create_account(
      const uint32_t seed,
      const string& name,
      const public_key_type& key = public_key_type()
      );
   const account_object& create_account(
      const account_uid_type uid,
      const string& name,
      const public_key_type& key = public_key_type()
      );

   const account_object& create_account(
      const account_uid_type uid,
      const string& name,
      const account_object& registrar,
      const account_object& referrer,
      uint8_t referrer_percent = 100,
      const public_key_type& key = public_key_type()
      );

   const account_object& create_account(
      const account_uid_type uid,
      const string& name,
      const private_key_type& key,
      const account_uid_type& registrar_id = GRAPHENE_NULL_ACCOUNT_UID,
      const account_uid_type& referrer_id = GRAPHENE_NULL_ACCOUNT_UID,
      uint8_t referrer_percent = 100
      );

   const committee_member_object& create_committee_member(const account_object& owner);
   const witness_object& create_witness(account_uid_type owner,
      const fc::ecc::private_key& signing_private_key = generate_private_key("null_key"), asset pledge = asset());
   const witness_object& create_witness(const account_object& owner,
      const fc::ecc::private_key& signing_private_key = generate_private_key("null_key"), asset pledge = asset());
   //   const worker_object& create_worker(account_id_type owner, const share_type daily_pay = 1000, const fc::microseconds& duration = fc::days(2));
   uint64_t fund(const account_object& account, const asset& amount = asset(500000));
   digest_type digest(const transaction& tx);
   void sign(signed_transaction& trx, const fc::ecc::private_key& key);
   //   const limit_order_object* create_sell_order( account_uid_type user, const asset& amount, const asset& recv );
   //   const limit_order_object* create_sell_order( const account_object& user, const asset& amount, const asset& recv );
   //   asset cancel_limit_order( const limit_order_object& order );
   void transfer(account_uid_type from, account_uid_type to, const asset& amount, const asset& fee = asset());
   void transfer(const account_object& from, const account_object& to, const asset& amount, const asset& fee = asset());
   void enable_fees();
   void change_fees(const flat_set< fee_parameters >& new_params, uint32_t new_scale = 0);
   //TODO:lifetime member
   void upgrade_to_lifetime_member(account_uid_type account);
   void upgrade_to_lifetime_member(const account_object& account);
   //TODO:annual member
   void upgrade_to_annual_member(account_uid_type account);
   void upgrade_to_annual_member(const account_object& account);
   void print_market(const string& syma, const string& symb)const;
   string pretty(const asset& a)const;
   //   void print_limit_order( const limit_order_object& cur )const;
   // TODO: market
   void print_joint_market(const string& syma, const string& symb)const;
   int64_t get_balance(account_uid_type account, asset_aid_type a)const;
   int64_t get_balance(const account_object& account, const asset_object& a)const;
   vector< operation_history_object > get_operation_history(account_uid_type account_id)const;

   //content test add
   void add_csaf_for_account(account_uid_type account, share_type csaf);
   //void collect_csaf(account_uid_type from, account_uid_type to, uint32_t amount, string asset_symbol = "YOYO");
   void committee_proposal_create(
      const account_uid_type committee_member_account,
      const vector<committee_proposal_item_type> items,
      const uint32_t voting_closing_block_num,
      optional<voting_opinion_type> proposer_opinion,
      const uint32_t execution_block_num = 0,
      const uint32_t expiration_block_num = 0
      );

   void committee_proposal_vote(
      const account_uid_type committee_member_account,
      const uint64_t proposal_number,
      const voting_opinion_type opinion
      );

   void create_platform(account_uid_type owner_account,
      string name,
      asset pledge_amount,
      string url,
      string extra_data,
      flat_set<fc::ecc::private_key> sign_keys);

   void update_platform_votes(account_uid_type voting_account,
      flat_set<account_uid_type> platforms_to_add,
      flat_set<account_uid_type> platforms_to_remove,
      flat_set<fc::ecc::private_key> sign_keys
      );

   void reward_post(account_uid_type from_account,
      account_uid_type platform,
      account_uid_type poster,
      post_pid_type post_pid,
      asset amount,
      flat_set<fc::ecc::private_key> sign_keys);

   void reward_post_proxy_by_platform(account_uid_type from_account,
      account_uid_type platform,
      account_uid_type poster,
      post_pid_type    post_pid,
      uint64_t         amount,
      flat_set<fc::ecc::private_key> sign_keys);

   void buyout_post(account_uid_type from_account,
      account_uid_type platform,
      account_uid_type poster,
      post_pid_type    post_pid,
      account_uid_type receiptor_account,
      optional<account_uid_type> sign_platform,
      flat_set<fc::ecc::private_key> sign_keys);

   void create_license(account_uid_type platform,
      uint8_t license_type,
      string  hash_value,
      string  title,
      string  body,
      string  extra_data,
      flat_set<fc::ecc::private_key> sign_keys);

   void set_operation_fees(signed_transaction& tx, const fee_schedule& s)
   {
      for (auto& op : tx.operations)
         s.set_fee_with_csaf(op);
   }

   void transfer_extension(flat_set<fc::ecc::private_key> sign_keys,
      account_uid_type from,
      account_uid_type to,
      asset            amount,
      string           memo,
      bool             isfrom_balance = true,
      bool             isto_balance = true);

   void account_auth_platform(flat_set<fc::ecc::private_key> sign_keys,
      account_uid_type account,
      account_uid_type platform_owner,
      share_type       limit_for_platform = 0,
      uint32_t         permission_flags = 0xFFFFFFFF);

   void create_post(flat_set<fc::ecc::private_key> sign_keys,
      account_uid_type           platform,
      account_uid_type           poster,
      string                     hash_value,
      string                     title,
      string                     body,
      string                     extra_data,
      optional<account_uid_type> origin_platform,
      optional<account_uid_type> origin_poster,
      optional<post_pid_type>    origin_post_pid,
      post_operation::ext  exts = post_operation::ext());

   void update_post(flat_set<fc::ecc::private_key> sign_keys,
      account_uid_type              platform,
      account_uid_type              poster,
      post_pid_type                 post_pid,
      optional<string>              hash_value,
      optional<string>              title,
      optional<string>              body,
      optional<string>              extra_data,
      optional<post_update_operation::ext> ext = post_update_operation::ext());

   void score_a_post(flat_set<fc::ecc::private_key> sign_keys,
      account_uid_type from_account,
      account_uid_type platform,
      account_uid_type poster,
      post_pid_type    post_pid,
      int8_t           score,
      share_type       csaf);

   void buy_advertising(flat_set<fc::ecc::private_key> sign_keys,
      account_uid_type      account,
      account_uid_type      platform,
      advertising_aid_type  advertising_aid,
      time_point_sec        start_time,
      uint32_t              buy_number,
      string                extra_data,
      string                memo);

   void confirm_advertising(flat_set<fc::ecc::private_key> sign_keys,
      account_uid_type               platform,
      advertising_aid_type           advertising_aid,
      advertising_order_oid_type     advertising_order_oid,
      bool                           comfirm);


   void account_manage(account_uid_type account, account_manage_operation::opt options);
   void account_manage(account_uid_type executor,
      account_uid_type account,
      account_manage_operation::opt options
      );

   void collect_csaf(flat_set<fc::ecc::private_key> sign_keys, account_uid_type from_account, account_uid_type to_account, int64_t amount);
   void collect_csaf_origin(flat_set<fc::ecc::private_key> sign_keys, account_uid_type from_account, account_uid_type to_account, int64_t amount);
   void collect_csaf_from_committee(account_uid_type to_account, int64_t amount);
   void csaf_lease(flat_set<fc::ecc::private_key> sign_keys, account_uid_type from_account, account_uid_type to_account, int64_t amount, time_point_sec expiration);

   void actor(uint32_t start, uint32_t limit, flat_map<account_uid_type, fc::ecc::private_key>& map)
   {
      while (limit > 0)
      {
         account_uid_type uid = graphene::chain::calc_account_uid(start);
         fc::ecc::private_key pri_key = generate_private_key(BOOST_PP_STRINGIZE(start));
         public_key_type pub_key = pri_key.get_public_key();
         char name[12];
         sprintf(name, "u%d", start);
         create_account(uid, name, pub_key);
         map.emplace(uid, pri_key);
         start++;
         limit--;
      }
   }

   void create_advertising(flat_set<fc::ecc::private_key> sign_keys,
      account_uid_type               platform,
      string                         description,
      share_type                     unit_price,
      uint32_t                       unit_time);

   void update_advertising(flat_set<fc::ecc::private_key> sign_keys,
      account_uid_type               platform,
      advertising_aid_type           advertising_aid,
      optional<string>               description,
      optional<share_type>           unit_price,
      optional<uint32_t>             unit_time,
      optional<bool>                 on_sell);

   void ransom_advertising(flat_set<fc::ecc::private_key> sign_keys,
      account_uid_type               platform,
      account_uid_type               from_account,
      advertising_aid_type           advertising_aoid,
      advertising_order_oid_type     advertising_order_oid);

   void create_custom_vote(flat_set<fc::ecc::private_key> sign_keys,
      account_uid_type create_account,
      custom_vote_vid_type  custom_vote_vid,
      string           title,
      string           description,
      time_point_sec   expired_time,
      asset_aid_type   asset_id,
      share_type       required_amount,
      uint8_t          minimum_selected_items,
      uint8_t          maximum_selected_items,
      vector<string>   options);

   void cast_custom_vote(flat_set<fc::ecc::private_key> sign_keys,
      account_uid_type      voter,
      account_uid_type      custom_vote_creator,
      custom_vote_vid_type  custom_vote_vid,
      set<uint8_t>          vote_result);

   void balance_lock_update(flat_set<fc::ecc::private_key> sign_keys, account_uid_type account, share_type amount);
   void update_mining_pledge(flat_set<fc::ecc::private_key> sign_keys,
      account_uid_type pledge_account,
      account_uid_type witness,
      uint64_t new_pledge);
   void create_witness(flat_set<fc::ecc::private_key> sign_keys,
      account_uid_type owner_account,
      string url,
      share_type pledge,
      graphene::chain::public_key_type signing_public_key,
      graphene::chain::pledge_mining::ext ext);

   void update_witness(flat_set<fc::ecc::private_key> sign_keys,
      account_uid_type account,
      optional<public_key_type> new_signing_key,
      optional<asset> new_pledge,
      optional<string> new_url
      );

   void create_committee(flat_set<fc::ecc::private_key> sign_keys,
      account_uid_type owner_account,
      string url,
      share_type pledge);

   void update_committee(flat_set<fc::ecc::private_key> sign_keys,
      account_uid_type owner_account,
      share_type pledge);

   void create_asset(flat_set<fc::ecc::private_key> sign_keys,
      account_uid_type issuer,
      string           symbol,
      uint8_t          precisions,
      asset_options    options,
      share_type       initial_supply,
      uint32_t         skip = 0
      );

   void update_asset(flat_set<fc::ecc::private_key> sign_keys,
      account_uid_type  issuer,
      asset_aid_type    asset_to_update,
      optional<uint8_t> new_precision,
      asset_options     new_options
      );

   void create_limit_order(flat_set<fc::ecc::private_key> sign_keys,
      account_uid_type seller,
      asset_aid_type   sell_asset_id,
      share_type       sell_amount,
      asset_aid_type   min_receive_asset_id,
      share_type       min_receive_amount,
      uint32_t         expiration,
      bool             fill_or_kill
      );

   void cancel_limit_order(flat_set<fc::ecc::private_key> sign_keys,
      account_uid_type     seller,
      limit_order_id_type  order_id
      );

   void collect_market_fee(flat_set<fc::ecc::private_key> sign_keys,
      account_uid_type     account,
      asset_aid_type       asset_aid,
      share_type           amount
      );

   void asset_claim_fees(flat_set<fc::ecc::private_key> sign_keys,
      account_uid_type     issuer,
      asset_aid_type       asset_aid,
      share_type           amount_to_claim
      );

   fc::uint128_t compute_coin_seconds_earned(_account_statistics_object account, share_type effective_balance, time_point_sec now_rounded);

   std::tuple<vector<std::tuple<account_uid_type, share_type, bool>>, share_type>
      get_effective_csaf(const vector<score_id_type>& scores, share_type amount);
};

namespace test {
/// set a reasonable expiration time for the transaction
void set_expiration( const database& db, transaction& tx );

bool _push_block( database& db, const signed_block& b, uint32_t skip_flags = 0 );
processed_transaction _push_transaction( database& db, const signed_transaction& tx, uint32_t skip_flags = 0 );
}

} }
