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

#include <iostream>

using namespace graphene::db;

extern uint32_t GRAPHENE_TESTING_GENESIS_TIMESTAMP;

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
   account_uid_type u_ ## seed = graphene::chain::calc_account_uid(seed); \
   const auto& uobj = create_account(u_ ## seed,BOOST_PP_STRINGIZE(seed), u_ ## seed ## _public_key); \
   (void)uobj;

#define GET_ACTOR(seed) \
   fc::ecc::private_key u_ ## seed ## _private_key = generate_private_key(BOOST_PP_STRINGIZE(seed)); \
   const account_object& u_ ## seed = get_account(BOOST_PP_STRINGIZE(seed)); \
   account_uid_type u_ ## seed ## _id = u_ ## seed.uid; \
   (void)u_ ##seed

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
   account_uid_type committee_account = 25638;
   fc::ecc::private_key private_key = fc::ecc::private_key::generate();
   fc::ecc::private_key init_account_priv_key = fc::ecc::private_key::regenerate(fc::sha256::hash(string("null_key")) );
   public_key_type init_account_pub_key;

   optional<fc::temp_directory> data_dir;
   bool skip_key_index_test = false;
   uint32_t anon_acct_count;

   database_fixture();
   ~database_fixture();

   static fc::ecc::private_key generate_private_key(string seed);
   string generate_anon_acct_name();
   static void verify_asset_supplies( const database& db );
   void verify_account_history_plugin_index( )const;
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

   const asset_object& get_asset( const string& symbol )const;
   const account_object& get_account( const string& name )const;

   const asset_object& create_user_issued_asset( const string& name );
   const asset_object& create_user_issued_asset( const string& name,
                                                 const account_object& issuer,
                                                 uint16_t flags );
   void issue_uia( const account_object& recipient, asset amount );
   void issue_uia( account_uid_type recipient_id, asset amount );

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

   const committee_member_object& create_committee_member( const account_object& owner );
   const witness_object& create_witness(account_uid_type owner,
                                        const fc::ecc::private_key& signing_private_key = generate_private_key("null_key"));
   const witness_object& create_witness(const account_object& owner,
                                        const fc::ecc::private_key& signing_private_key = generate_private_key("null_key"));
   uint64_t fund( const account_object& account, const asset& amount = asset(500000) );
   digest_type digest( const transaction& tx );
   void sign( signed_transaction& trx, const fc::ecc::private_key& key );
   const limit_order_object* create_sell_order( account_uid_type user, const asset& amount, const asset& recv );
   const limit_order_object* create_sell_order( const account_object& user, const asset& amount, const asset& recv );
   asset cancel_limit_order( const limit_order_object& order );
   void transfer( account_uid_type from, account_uid_type to, const asset& amount, const asset& fee = asset() );
   void transfer( const account_object& from, const account_object& to, const asset& amount, const asset& fee = asset() );
   void enable_fees();
   void change_fees( const flat_set< fee_parameters >& new_params, uint32_t new_scale = 0 );
   //TODO:lifetime member
   void upgrade_to_lifetime_member( account_uid_type account );
   void upgrade_to_lifetime_member( const account_object& account );
   //TODO:annual member
   void upgrade_to_annual_member( account_uid_type account );
   void upgrade_to_annual_member( const account_object& account );
   void print_market( const string& syma, const string& symb )const;
   string pretty( const asset& a )const;
   void print_limit_order( const limit_order_object& cur )const;
   // TODO: market
   void print_joint_market( const string& syma, const string& symb )const;
   int64_t get_balance( account_uid_type account, asset_aid_type a )const;
   int64_t get_balance( const account_object& account, const asset_object& a )const;
   vector< operation_history_object > get_operation_history( account_uid_type account_id )const;
};

namespace test {
/// set a reasonable expiration time for the transaction
void set_expiration( const database& db, transaction& tx );

bool _push_block( database& db, const signed_block& b, uint32_t skip_flags = 0 );
processed_transaction _push_transaction( database& db, const signed_transaction& tx, uint32_t skip_flags = 0 );
}

} }
