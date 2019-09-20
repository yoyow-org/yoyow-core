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
#include <fc/container/flat_fwd.hpp>
#include <fc/io/varint.hpp>
#include <fc/io/enum_type.hpp>
#include <fc/crypto/sha224.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/optional.hpp>
#include <fc/safe.hpp>
#include <fc/container/flat.hpp>
#include <fc/string.hpp>

#include <graphene/chain/protocol/ext.hpp>

#include <fc/io/raw.hpp>
#include <fc/uint128.hpp>
#include <fc/static_variant.hpp>
#include <fc/smart_ref_fwd.hpp>

#include <memory>
#include <vector>
#include <deque>
#include <cstdint>
#include <graphene/db/object_id.hpp>
#include <graphene/chain/protocol/config.hpp>

namespace graphene { namespace chain {
   using namespace graphene::db;

   using                               std::map;
   using                               std::vector;
   using                               std::unordered_map;
   using                               std::string;
   using                               std::deque;
   using                               std::shared_ptr;
   using                               std::weak_ptr;
   using                               std::unique_ptr;
   using                               std::set;
   using                               std::pair;
   using                               std::enable_shared_from_this;
   using                               std::tie;
   using                               std::make_pair;

   using                               fc::smart_ref;
   using                               fc::variant_object;
   using                               fc::variant;
   using                               fc::enum_type;
   using                               fc::optional;
   using                               fc::unsigned_int;
   using                               fc::signed_int;
   using                               fc::time_point_sec;
   using                               fc::time_point;
   using                               fc::safe;
   using                               fc::flat_map;
   using                               fc::flat_set;
   using                               fc::static_variant;
   using                               fc::ecc::range_proof_type;
   using                               fc::ecc::range_proof_info;
   using                               fc::ecc::commitment_type;
   struct void_t{};

   typedef fc::ecc::private_key        private_key_type;
   typedef fc::sha256                  chain_id_type;

   typedef uint64_t                    account_uid_type;
   typedef uint64_t                    asset_aid_type;
   typedef uint64_t                    post_pid_type;
   typedef uint64_t                    license_lid_type;
   typedef uint64_t                    committee_proposal_number_type;
   typedef uint64_t                    advertising_aid_type;
   typedef uint64_t                    advertising_order_oid_type;
   typedef uint64_t                    custom_vote_vid_type;

   account_uid_type                    calc_account_uid( uint64_t id_without_checksum );
   bool                                is_valid_account_uid( const account_uid_type uid );


   // NOTE: permissions/flags are definded as uint16_t, aka at max 0xFFFF
   typedef uint16_t                    asset_flags_type;
   enum asset_issuer_permission_flags
   {
      charge_market_fee    = 0x01, /**< an issuer-specified percentage of all market trades in this asset is paid to the issuer */
      white_list           = 0x02, /**< accounts must be whitelisted in order to hold this asset */
      override_authority   = 0x04, /**< issuer may transfer asset back to himself */
      transfer_restricted  = 0x08, /**< require the issuer to be one party to every transfer */
      //disable_force_settle = 0x10, /**< disable force settling */
      //global_settle        = 0x20, /**< allow the bitasset issuer to force a global settling -- this may be set in permissions, but not flags */
      //disable_confidential = 0x40 //, /**< allow the asset to be used with confidential transactions */
      //witness_fed_asset    = 0x80, /**< allow the asset to be fed by witnesses */
      //committee_fed_asset  = 0x100 /**< allow the asset to be fed by the committee */
      issue_asset          = 0x200, /**< allow the issuer to create an amount of the asset (increase current supply) */
      change_max_supply    = 0x400, /**< allow the issuer to change the asset's max supply */
   };
   //const static uint32_t ASSET_ISSUER_PERMISSION_MASK = charge_market_fee|white_list|override_authority|transfer_restricted|disable_force_settle|global_settle|disable_confidential|witness_fed_asset|committee_fed_asset;

   //const static uint32_t UIA_ASSET_ISSUER_PERMISSION_MASK = charge_market_fee|white_list|override_authority|transfer_restricted|disable_confidential;

   const static asset_flags_type ASSET_ISSUER_PERMISSION_MASK = charge_market_fee | white_list | override_authority | transfer_restricted | issue_asset | change_max_supply;

   enum scheduled_witness_type
   {
      scheduled_by_vote_top  = 0,
      scheduled_by_vote_rest = 1,
      scheduled_by_pledge    = 2
   };

   enum reserved_spaces
   {
      relative_protocol_ids = 0,
      protocol_ids          = 1,
      implementation_ids    = 2
   };

   inline bool is_relative( object_id_type o ){ return o.space() == 0; }

   /**
    *  List all object types from all namespaces here so they can
    *  be easily reflected and displayed in debug output.  If a 3rd party
    *  wants to extend the core code then they will have to change the
    *  packed_object::type field from enum_type to uint16 to avoid
    *  warnings when converting packed_objects to/from json.
    */
   enum object_type
   {
      null_object_type,
      base_object_type,
      account_object_type,
      asset_object_type,
      committee_member_object_type,
      witness_object_type,
      platform_object_type,
      post_object_type,
      committee_proposal_object_type,
      proposal_object_type,
      operation_history_object_type,
      active_post_object_type,
      limit_order_object_type,
      OBJECT_TYPE_COUNT ///< Sentry value which contains the number of different object types
   };

   enum impl_object_type
   {
      impl_global_property_object_type,
      impl_dynamic_global_property_object_type,
      impl_asset_dynamic_data_type,
      impl_account_balance_object_type,
      impl_account_statistics_object_type,
      impl_voter_object_type,
      impl_witness_vote_object_type,
      impl_committee_member_vote_object_type,
      impl_registrar_takeover_object_type,
      impl_csaf_lease_object_type,
      impl_transaction_object_type,
      impl_block_summary_object_type,
      impl_account_transaction_history_object_type,
      impl_chain_property_object_type,
      impl_witness_schedule_object_type,
      impl_platform_vote_object_type,
      impl_score_object_type,
      impl_license_object_type,
      impl_advertising_object_type,
      impl_advertising_order_object_type,
      impl_custom_vote_object_type,
      impl_cast_custom_vote_object_type,
      impl_account_auth_platform_object_type,
      impl_pledge_mining_object_type,
      impl_pledge_balance_object_type,
      IMPL_OBJECT_TYPE_COUNT ///< Sentry value which contains the number of different impl object types
   };

   class account_object;
   class platform_object;
   class post_object;
   class committee_member_object;
   class witness_object;
   class asset_object;
   class committee_proposal_object;
   class proposal_object;
   class operation_history_object;
   class active_post_object;
   class license_object;
   class advertising_object;
   class advertising_order_object;
   class custom_vote_object;
   class cast_custom_vote_object;
   class account_auth_platform_object;
   class pledge_mining_object;
   class limit_order_object;

   typedef object_id< protocol_ids, account_object_type,            account_object>               account_id_type;
   typedef object_id< protocol_ids, asset_object_type,              asset_object>                 asset_id_type;
   typedef object_id< protocol_ids, committee_member_object_type,   committee_member_object>      committee_member_id_type;
   typedef object_id< protocol_ids, witness_object_type,            witness_object>               witness_id_type;
   typedef object_id< protocol_ids, platform_object_type,           platform_object>              platform_id_type;
   typedef object_id< protocol_ids, post_object_type,               post_object>                  post_id_type;
   typedef object_id< protocol_ids, committee_proposal_object_type, committee_proposal_object>    committee_proposal_id_type;
   typedef object_id< protocol_ids, proposal_object_type,           proposal_object>              proposal_id_type;
   typedef object_id< protocol_ids, operation_history_object_type,  operation_history_object>     operation_history_id_type;
   typedef object_id< protocol_ids, active_post_object_type,		  active_post_object>			  active_post_id_type;
   typedef object_id< protocol_ids, limit_order_object_type,        limit_order_object>           limit_order_id_type;

   // implementation types
   class global_property_object;
   class dynamic_global_property_object;
   class asset_dynamic_data_object;
   class account_balance_object;
   class account_statistics_object;
   class voter_object;
   class witness_vote_object;
   class platform_vote_object;
   class committee_member_vote_object;
   class registrar_takeover_object;
   class csaf_lease_object;
   class transaction_object;
   class block_summary_object;
   class account_transaction_history_object;
   class chain_property_object;
   class witness_schedule_object;
   class score_object;
   class pledge_balance_object;

   typedef object_id< implementation_ids, impl_global_property_object_type,  global_property_object>                    global_property_id_type;
   typedef object_id< implementation_ids, impl_dynamic_global_property_object_type,  dynamic_global_property_object>    dynamic_global_property_id_type;
   typedef object_id< implementation_ids, impl_asset_dynamic_data_type,      asset_dynamic_data_object>                 asset_dynamic_data_id_type;
   typedef object_id< implementation_ids, impl_account_balance_object_type,  account_balance_object>                    account_balance_id_type;
   typedef object_id< implementation_ids, impl_account_statistics_object_type,account_statistics_object>                account_statistics_id_type;
   typedef object_id< implementation_ids, impl_voter_object_type,            voter_object>                              voter_id_type;
   typedef object_id< implementation_ids, impl_witness_vote_object_type,     witness_vote_object>                       witness_vote_id_type;
   typedef object_id< implementation_ids, impl_committee_member_vote_object_type,     committee_member_vote_object>     committee_member_vote_id_type;
   typedef object_id< implementation_ids, impl_registrar_takeover_object_type, registrar_takeover_object>               registrar_takeover_id_type;
   typedef object_id< implementation_ids, impl_csaf_lease_object_type,       csaf_lease_object>                         csaf_lease_id_type;
   typedef object_id< implementation_ids, impl_transaction_object_type,      transaction_object>                        transaction_obj_id_type;
   typedef object_id< implementation_ids, impl_block_summary_object_type,    block_summary_object>                      block_summary_id_type;

   typedef object_id< implementation_ids,
                      impl_account_transaction_history_object_type,
                      account_transaction_history_object>       account_transaction_history_id_type;
   typedef object_id< implementation_ids, impl_chain_property_object_type,   chain_property_object>                     chain_property_id_type;
   typedef object_id< implementation_ids, impl_witness_schedule_object_type, witness_schedule_object>                   witness_schedule_id_type;
   typedef object_id< implementation_ids, impl_platform_vote_object_type,    platform_vote_object>                      platform_vote_id_type;
   typedef object_id< implementation_ids, impl_score_object_type,            score_object>                              score_id_type;
   typedef object_id< implementation_ids, impl_license_object_type,          license_object>                            license_id_type;
   typedef object_id< implementation_ids, impl_advertising_object_type,      advertising_object>                        advertising_id_type;
   typedef object_id< implementation_ids, impl_advertising_order_object_type,advertising_order_object>                  advertising_order_id_type;
   typedef object_id< implementation_ids, impl_custom_vote_object_type,      custom_vote_object>                        custom_vote_id_type;
   typedef object_id< implementation_ids, impl_cast_custom_vote_object_type, cast_custom_vote_object>                   cast_custom_vote_id_type;
   typedef object_id< implementation_ids, impl_account_auth_platform_object_type, account_auth_platform_object>         account_auth_platform_id_type;
   typedef object_id< implementation_ids, impl_pledge_mining_object_type,    pledge_mining_object>                      pledge_mining_id_type;
   typedef object_id< implementation_ids, impl_pledge_balance_object_type,   pledge_balance_object>                     pledge_balance_id_type;

   typedef fc::array<char, GRAPHENE_MAX_ASSET_SYMBOL_LENGTH>    symbol_type;
   typedef fc::ripemd160                                        block_id_type;
   typedef fc::ripemd160                                        checksum_type;
   typedef fc::ripemd160                                        transaction_id_type;
   typedef fc::sha256                                           digest_type;
   typedef fc::ecc::compact_signature                           signature_type;
   typedef safe<int64_t>                                        share_type;
   typedef uint16_t                                             weight_type;

   struct public_key_type
   {
       struct binary_key
       {
          binary_key() {}
          uint32_t                 check = 0;
          fc::ecc::public_key_data data;
       };
       fc::ecc::public_key_data key_data;
       public_key_type();
       public_key_type( const fc::ecc::public_key_data& data );
       public_key_type( const fc::ecc::public_key& pubkey );
       explicit public_key_type( const std::string& base58str );
       operator fc::ecc::public_key_data() const;
       operator fc::ecc::public_key() const;
       explicit operator std::string() const;
       friend bool operator == ( const public_key_type& p1, const fc::ecc::public_key& p2);
       friend bool operator == ( const public_key_type& p1, const public_key_type& p2);
       friend bool operator != ( const public_key_type& p1, const public_key_type& p2);
       friend bool operator <  ( const public_key_type& p1, const public_key_type& p2);
   };

   struct extended_public_key_type
   {
      struct binary_key
      {
         binary_key() {}
         uint32_t                   check = 0;
         fc::ecc::extended_key_data data;
      };

      fc::ecc::extended_key_data key_data;

      extended_public_key_type();
      extended_public_key_type( const fc::ecc::extended_key_data& data );
      extended_public_key_type( const fc::ecc::extended_public_key& extpubkey );
      explicit extended_public_key_type( const std::string& base58str );
      operator fc::ecc::extended_public_key() const;
      explicit operator std::string() const;
      friend bool operator == ( const extended_public_key_type& p1, const fc::ecc::extended_public_key& p2);
      friend bool operator == ( const extended_public_key_type& p1, const extended_public_key_type& p2);
      friend bool operator != ( const extended_public_key_type& p1, const extended_public_key_type& p2);
   };

   struct extended_private_key_type
   {
      struct binary_key
      {
         binary_key() {}
         uint32_t                   check = 0;
         fc::ecc::extended_key_data data;
      };

      fc::ecc::extended_key_data key_data;

      extended_private_key_type();
      extended_private_key_type( const fc::ecc::extended_key_data& data );
      extended_private_key_type( const fc::ecc::extended_private_key& extprivkey );
      explicit extended_private_key_type( const std::string& base58str );
      operator fc::ecc::extended_private_key() const;
      explicit operator std::string() const;
      friend bool operator == ( const extended_private_key_type& p1, const fc::ecc::extended_private_key& p2);
      friend bool operator == ( const extended_private_key_type& p1, const extended_private_key_type& p2);
      friend bool operator != ( const extended_private_key_type& p1, const extended_private_key_type& p2);
   };

   enum ENABLE_HEAD_FORK_TYPE{
      ENABLE_HEAD_FORK_NONE = 0,
      ENABLE_HEAD_FORK_04 = 1,
      ENABLE_HEAD_FORK_05 = 2,

      ENABLE_HEAD_FORK_NUM
   };
} }  // graphene::chain

namespace fc
{
    void to_variant( const graphene::chain::public_key_type& var,  fc::variant& vo, uint32_t max_depth = 2 );
    void from_variant( const fc::variant& var,  graphene::chain::public_key_type& vo, uint32_t max_depth = 2 );
    void to_variant( const graphene::chain::extended_public_key_type& var, fc::variant& vo, uint32_t max_depth = 2 );
    void from_variant( const fc::variant& var, graphene::chain::extended_public_key_type& vo, uint32_t max_depth = 2 );
    void to_variant( const graphene::chain::extended_private_key_type& var, fc::variant& vo, uint32_t max_depth = 2 );
    void from_variant( const fc::variant& var, graphene::chain::extended_private_key_type& vo, uint32_t max_depth = 2 );
}

FC_REFLECT( graphene::chain::public_key_type, (key_data) )
FC_REFLECT( graphene::chain::public_key_type::binary_key, (data)(check) )
FC_REFLECT( graphene::chain::extended_public_key_type, (key_data) )
FC_REFLECT( graphene::chain::extended_public_key_type::binary_key, (check)(data) )
FC_REFLECT( graphene::chain::extended_private_key_type, (key_data) )
FC_REFLECT( graphene::chain::extended_private_key_type::binary_key, (check)(data) )

FC_REFLECT_ENUM( graphene::chain::object_type,
                 (null_object_type)
                 (base_object_type)
                 (account_object_type)
                 (asset_object_type)
                 (committee_member_object_type)
                 (witness_object_type)
                 (platform_object_type)
                 (post_object_type)
                 (committee_proposal_object_type)
                 (proposal_object_type)
                 (operation_history_object_type)
                 (active_post_object_type)
                 (limit_order_object_type)
                 (OBJECT_TYPE_COUNT)
               )
FC_REFLECT_ENUM( graphene::chain::impl_object_type,
                 (impl_global_property_object_type)
                 (impl_dynamic_global_property_object_type)
                 (impl_asset_dynamic_data_type)
                 (impl_account_balance_object_type)
                 (impl_account_statistics_object_type)
                 (impl_voter_object_type)
                 (impl_witness_vote_object_type)
                 (impl_committee_member_vote_object_type)
                 (impl_registrar_takeover_object_type)
                 (impl_csaf_lease_object_type)
                 (impl_transaction_object_type)
                 (impl_block_summary_object_type)
                 (impl_account_transaction_history_object_type)
                 (impl_chain_property_object_type)
                 (impl_witness_schedule_object_type)
                 (impl_platform_vote_object_type)
                 (impl_score_object_type)
                 (impl_license_object_type)
                 (impl_advertising_object_type)
                 (impl_advertising_order_object_type)
                 (impl_custom_vote_object_type)
                 (impl_cast_custom_vote_object_type)
                 (impl_account_auth_platform_object_type)
                 (impl_pledge_mining_object_type)
                 (impl_pledge_balance_object_type)
                 (IMPL_OBJECT_TYPE_COUNT)
               )

FC_REFLECT_TYPENAME( graphene::chain::share_type )

FC_REFLECT_TYPENAME( graphene::chain::account_id_type )
FC_REFLECT_TYPENAME( graphene::chain::asset_id_type )
FC_REFLECT_TYPENAME( graphene::chain::platform_id_type )
FC_REFLECT_TYPENAME( graphene::chain::post_id_type )
FC_REFLECT_TYPENAME( graphene::chain::committee_member_id_type )
FC_REFLECT_TYPENAME( graphene::chain::witness_id_type )
FC_REFLECT_TYPENAME( graphene::chain::committee_proposal_id_type )
FC_REFLECT_TYPENAME( graphene::chain::proposal_id_type )
FC_REFLECT_TYPENAME( graphene::chain::operation_history_id_type )
FC_REFLECT_TYPENAME( graphene::chain::active_post_id_type)
FC_REFLECT_TYPENAME( graphene::chain::limit_order_id_type)

FC_REFLECT_TYPENAME( graphene::chain::global_property_id_type )
FC_REFLECT_TYPENAME( graphene::chain::dynamic_global_property_id_type )
FC_REFLECT_TYPENAME( graphene::chain::asset_dynamic_data_id_type )
FC_REFLECT_TYPENAME( graphene::chain::account_balance_id_type )
FC_REFLECT_TYPENAME( graphene::chain::account_statistics_id_type )
FC_REFLECT_TYPENAME( graphene::chain::voter_id_type )
FC_REFLECT_TYPENAME( graphene::chain::witness_vote_id_type )
FC_REFLECT_TYPENAME( graphene::chain::platform_vote_id_type)
FC_REFLECT_TYPENAME( graphene::chain::score_id_type)
FC_REFLECT_TYPENAME( graphene::chain::license_id_type)
FC_REFLECT_TYPENAME( graphene::chain::advertising_id_type)
FC_REFLECT_TYPENAME( graphene::chain::advertising_order_id_type)
FC_REFLECT_TYPENAME( graphene::chain::custom_vote_id_type)
FC_REFLECT_TYPENAME( graphene::chain::cast_custom_vote_id_type)
FC_REFLECT_TYPENAME( graphene::chain::account_auth_platform_id_type)
FC_REFLECT_TYPENAME( graphene::chain::committee_member_vote_id_type )
FC_REFLECT_TYPENAME( graphene::chain::registrar_takeover_id_type )
FC_REFLECT_TYPENAME( graphene::chain::csaf_lease_id_type )
FC_REFLECT_TYPENAME( graphene::chain::transaction_obj_id_type )
FC_REFLECT_TYPENAME( graphene::chain::block_summary_id_type )
FC_REFLECT_TYPENAME( graphene::chain::account_transaction_history_id_type )
FC_REFLECT_TYPENAME( graphene::chain::pledge_mining_id_type)
FC_REFLECT_TYPENAME( graphene::chain::pledge_balance_id_type)

FC_REFLECT( graphene::chain::void_t, )

FC_REFLECT_ENUM( graphene::chain::asset_issuer_permission_flags,
   //(charge_market_fee)
   (white_list)
   (transfer_restricted)
   (override_authority)
   //(disable_force_settle)
   //(global_settle)
   //(disable_confidential)
   //(witness_fed_asset)
   //(committee_fed_asset)
   (issue_asset)
   (change_max_supply)
   )

FC_REFLECT_ENUM( graphene::chain::scheduled_witness_type,
   (scheduled_by_vote_top)
   (scheduled_by_vote_rest)
   (scheduled_by_pledge)
   )
