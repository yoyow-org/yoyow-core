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

#include <graphene/chain/protocol/chain_parameters.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/immutable_chain_parameters.hpp>

#include <fc/crypto/sha256.hpp>

#include <string>
#include <vector>

namespace graphene { namespace chain {
using std::string;
using std::vector;

struct genesis_state_type {
   struct initial_account_type {
      initial_account_type(const account_uid_type _uid = 0,
                           const string& _name = string(),
                           const account_uid_type _registrar = 0,
                           const public_key_type& _owner_key = public_key_type(),
                           const public_key_type& _active_key = public_key_type(),
                           const public_key_type& _secondary_key = public_key_type(),
                           const public_key_type& _memo_key = public_key_type(),
                           bool _is_lifetime_member = false,
                           bool _is_registrar = false,
                           bool _is_full_member = false)
         : uid(_uid),
           name(_name),
           registrar(_registrar),
           owner_key(_owner_key),
           active_key(_active_key == public_key_type()? _owner_key : _active_key),
           secondary_key(_secondary_key == public_key_type()? _owner_key : _secondary_key),
           memo_key(_memo_key == public_key_type()? _owner_key : _memo_key),
           is_lifetime_member(_is_lifetime_member),
           is_registrar(_is_registrar),
           is_full_member(_is_full_member)
      {}
      account_uid_type uid;
      string name;
      account_uid_type registrar;
      public_key_type owner_key;
      public_key_type active_key;
      public_key_type secondary_key;
      public_key_type memo_key;
      bool is_lifetime_member = false;
      bool is_registrar = false;
      bool is_full_member = false;
   };
   struct initial_asset_type {
      string symbol;
      string issuer_name;

      string description;
      uint8_t precision = GRAPHENE_BLOCKCHAIN_PRECISION_DIGITS;

      share_type max_supply;
      share_type accumulated_fees;
   };
   struct initial_account_balance_type {
     initial_account_balance_type(const account_uid_type _uid = 0,
                                  const string _asset_symbol = GRAPHENE_SYMBOL,
                                  const share_type _amount = 0)
                      :uid(_uid),
                      asset_symbol(_asset_symbol),
                      amount(_amount)
     {}
      account_uid_type uid;
      string asset_symbol;
      share_type amount;
   };
   struct initial_witness_type {
      /// Must correspond to one of the initial accounts
      string owner_name;
      public_key_type block_signing_key;
   };
   struct initial_committee_member_type {
      /// Must correspond to one of the initial accounts
      string owner_name;
   };
   struct initial_platform_type {
      /// Must correspond to one of the initial accounts
      account_uid_type owner;
      string name;
      string url;
   };

   time_point_sec                           initial_timestamp;
   share_type                               max_core_supply = GRAPHENE_MAX_SHARE_SUPPLY;
   chain_parameters                         initial_parameters;
   immutable_chain_parameters               immutable_parameters;
   vector<initial_account_type>             initial_accounts;
   vector<initial_asset_type>               initial_assets;
   vector<initial_account_balance_type>     initial_account_balances;
   uint64_t                                 initial_active_witnesses = GRAPHENE_DEFAULT_MIN_WITNESS_COUNT;
   vector<initial_witness_type>             initial_witness_candidates;
   vector<initial_committee_member_type>    initial_committee_candidates;
   vector<initial_platform_type>            initial_platforms;

   /**
    * Temporary, will be moved elsewhere.
    */
   chain_id_type                            initial_chain_id;

   /**
    * Get the chain_id corresponding to this genesis state.
    *
    * This is the SHA256 serialization of the genesis_state.
    */
   chain_id_type compute_chain_id() const;

   /// Method to override initial witness signing keys for debug
   void override_witness_signing_keys( const std::string& new_key );
};

} } // namespace graphene::chain

FC_REFLECT(graphene::chain::genesis_state_type::initial_account_type,
           (uid)(name)(registrar)(owner_key)(active_key)(secondary_key)(memo_key)(is_lifetime_member)(is_registrar)(is_full_member))

FC_REFLECT(graphene::chain::genesis_state_type::initial_asset_type,
           (symbol)(issuer_name)(description)(precision)(max_supply)(accumulated_fees))

FC_REFLECT(graphene::chain::genesis_state_type::initial_account_balance_type,
           (uid)(asset_symbol)(amount))

FC_REFLECT(graphene::chain::genesis_state_type::initial_witness_type, (owner_name)(block_signing_key))

FC_REFLECT(graphene::chain::genesis_state_type::initial_committee_member_type, (owner_name))

FC_REFLECT(graphene::chain::genesis_state_type::initial_platform_type, (owner)(name)(url))

FC_REFLECT(graphene::chain::genesis_state_type,
           (initial_timestamp)(max_core_supply)(initial_parameters)(initial_accounts)(initial_assets)
           (initial_account_balances)
           (initial_active_witnesses)(initial_witness_candidates)
           (initial_committee_candidates)
           (initial_platforms)
           (initial_chain_id)
           (immutable_parameters))
