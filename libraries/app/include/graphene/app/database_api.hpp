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

#include <graphene/app/full_account.hpp>

#include <graphene/chain/protocol/types.hpp>

#include <graphene/chain/database.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/chain_property_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/csaf_object.hpp>
#include <graphene/chain/operation_history_object.hpp>
#include <graphene/chain/proposal_object.hpp>
#include <graphene/chain/witness_object.hpp>

#include <fc/api.hpp>
#include <fc/optional.hpp>
#include <fc/variant_object.hpp>

#include <fc/network/ip.hpp>

#include <boost/container/flat_set.hpp>

#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace graphene { namespace app {

using namespace graphene::chain;
using namespace std;

class database_api_impl;

struct required_fee_data
{
   account_uid_type fee_payer_uid;
   int64_t          min_fee;
   int64_t          min_real_fee;
};

struct full_account_query_options
{
   optional<bool> fetch_account_object;
   optional<bool> fetch_statistics;
   optional<bool> fetch_csaf_leases_in;
   optional<bool> fetch_csaf_leases_out;
   optional<bool> fetch_voter_object;
   optional<bool> fetch_witness_object;
   optional<bool> fetch_witness_votes;
   optional<bool> fetch_committee_member_object;
   optional<bool> fetch_committee_member_votes;
   optional<bool> fetch_assets;
   optional<bool> fetch_balances;
};

enum data_sorting_type
{
   order_by_uid = 0,
   order_by_votes = 1,
   order_by_pledge = 2
};

struct signed_block_with_info : public signed_block
{
   signed_block_with_info(){}
   signed_block_with_info( const signed_block& block );
   signed_block_with_info( const signed_block_with_info& block ) = default;

   block_id_type block_id;
   public_key_type signing_key;
   vector< transaction_id_type > transaction_ids;
};

struct asset_object_with_data : public asset_object
{
   asset_object_with_data() {}
   asset_object_with_data( const asset_object& a ) : asset_object( a ) {}

   asset_dynamic_data_object dynamic_asset_data;
};

/**
 * @brief The database_api class implements the RPC API for the chain database.
 *
 * This API exposes accessors on the database which query state tracked by a blockchain validating node. This API is
 * read-only; all modifications to the database must be performed via transactions. Transactions are broadcast via
 * the @ref network_broadcast_api.
 */
class database_api
{
   public:
      database_api(graphene::chain::database& db);
      ~database_api();

      /////////////
      // Objects //
      /////////////

      /**
       * @brief Get the objects corresponding to the provided IDs
       * @param ids IDs of the objects to retrieve
       * @return The objects retrieved, in the order they are mentioned in ids
       *
       * If any of the provided IDs does not map to an object, a null variant is returned in its position.
       */
      fc::variants get_objects(const vector<object_id_type>& ids)const;

      ///////////////////
      // Subscriptions //
      ///////////////////

      void set_subscribe_callback( std::function<void(const variant&)> cb, bool clear_filter );
      void set_pending_transaction_callback( std::function<void(const variant&)> cb );
      void set_block_applied_callback( std::function<void(const variant& block_id)> cb );
      /**
       * @brief Stop receiving any notifications
       *
       * This unsubscribes from all subscribed markets and objects.
       */
      void cancel_all_subscriptions();

      /////////////////////////////
      // Blocks and transactions //
      /////////////////////////////

      /**
       * @brief Retrieve a block header
       * @param block_num Height of the block whose header should be returned
       * @return header of the referenced block, or null if no matching block was found
       */
      optional<block_header> get_block_header(uint32_t block_num)const;

      /**
      * @brief Retrieve multiple block header by block numbers
      * @param block_num vector containing heights of the block whose header should be returned
      * @return array of headers of the referenced blocks, or null if no matching block was found
      */
      map<uint32_t, optional<block_header>> get_block_header_batch(const vector<uint32_t> block_nums)const;


      /**
       * @brief Retrieve a full, signed block
       * @param block_num Height of the block to be returned
       * @return the referenced block, or null if no matching block was found
       */
      optional<signed_block_with_info> get_block(uint32_t block_num)const;

      /**
       * @brief used to fetch an individual transaction.
       */
      processed_transaction get_transaction( uint32_t block_num, uint32_t trx_in_block )const;

      /**
       * If the transaction has not expired, this method will return the transaction for the given ID or
       * it will return NULL if it is not known.  Just because it is not known does not mean it wasn't
       * included in the blockchain.
       */
      optional<signed_transaction> get_recent_transaction_by_id( const transaction_id_type& id )const;

      /////////////
      // Globals //
      /////////////

      /**
       * @brief Retrieve the @ref chain_property_object associated with the chain
       */
      chain_property_object get_chain_properties()const;

      /**
       * @brief Retrieve the current @ref global_property_object
       */
      global_property_object get_global_properties()const;

      /**
       * @brief Retrieve compile-time constants
       */
      fc::variant_object get_config()const;

      /**
       * @brief Get the chain ID
       */
      chain_id_type get_chain_id()const;

      /**
       * @brief Retrieve the current @ref dynamic_global_property_object
       */
      dynamic_global_property_object get_dynamic_global_properties()const;

      //////////
      // Keys //
      //////////

      vector<vector<account_uid_type>> get_key_references( vector<public_key_type> key )const;

     /**
      * Determine whether a textual representation of a public key
      * (in Base-58 format) is *currently* linked
      * to any *registered* (i.e. non-stealth) account on the blockchain
      * @param public_key Public key
      * @return Whether a public key is known
      */
     bool is_public_key_registered(string public_key) const;

      //////////////
      // Accounts //
      //////////////

      /**
       * @brief Get a list of accounts by ID
       * @param account_ids IDs of the accounts to retrieve
       * @return The accounts corresponding to the provided IDs
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<account_object>> get_accounts(const vector<account_id_type>& account_ids)const;

      /**
       * @brief Get a list of accounts by UID
       * @param account_uids UIDs of the accounts to retrieve
       * @return The accounts corresponding to the provided UIDs
       */
      vector<optional<account_object>> get_accounts_by_uid(const vector<account_uid_type>& account_uids)const;

      /**
       * @brief Fetch all objects relevant to the specified accounts and subscribe to updates
       * @param callback Function to call with updates
       * @param names_or_ids Each item must be the name or ID of an account to retrieve
       * @return Map of string from @ref names_or_ids to the corresponding account
       *
       * This function fetches all relevant objects for the given accounts, and subscribes to updates to the given
       * accounts. If any of the strings in @ref names_or_ids cannot be tied to an account, that input will be
       * ignored. All other accounts will be retrieved and subscribed.
       *
       */
      std::map<string,full_account> get_full_accounts( const vector<string>& names_or_ids, bool subscribe );

      /**
       * @brief Fetch all objects relevant to the specified accounts
       * @param uids Each item must be the UID of an account to retrieve
       * @param options Which items to fetch
       * @return Map of uid to the corresponding account
       *
       * This function fetches  relevant objects for the given accounts.
       * If any of the uids in @ref uids cannot be tied to an account, that input will be
       * ignored. All other accounts will be retrieved.
       *
       */
      std::map<account_uid_type,full_account> get_full_accounts_by_uid( const vector<account_uid_type>& uids,
                                                                        const full_account_query_options& options );

      /**
       * @brief Get an account by name
       * @param name name of the account to retrieve
       * @return The account corresponding to the provided name, or null if not found
       */
      optional<account_object> get_account_by_name( string name )const;

      /**
       *  @return all accounts that referr to the key or account id in their owner or active authorities.
       */
      vector<account_uid_type> get_account_references( account_uid_type uid )const;

      /**
       * @brief Get a list of accounts by name
       * @param account_names Names of the accounts to retrieve
       * @return The accounts holding the provided names
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<account_object>> lookup_account_names(const vector<string>& account_names)const;

      /**
       * @brief Get names and UIDs for registered accounts
       * @param lower_bound_name Lower bound of the first name to return
       * @param limit Maximum number of results to return -- must not exceed 1001
       * @return Map of account names to corresponding IDs
       */
      map<string,account_uid_type> lookup_accounts_by_name(const string& lower_bound_name, uint32_t limit)const;

      //////////////
      // Balances //
      //////////////

      /**
       * @brief Get an account's balances in various assets
       * @param id ID of the account to get balances for
       * @param assets IDs of the assets to get balances of; if empty, get all assets account has a balance in
       * @return Balances of the account
       */
      vector<asset> get_account_balances(account_uid_type uid, const flat_set<asset_aid_type>& assets)const;

      /// Semantically equivalent to @ref get_account_balances, but takes a name instead of an ID.
      vector<asset> get_named_account_balances(const std::string& name, const flat_set<asset_aid_type>& assets)const;

      /**
       * @brief Get the total number of accounts registered with the blockchain
       */
      uint64_t get_account_count()const;

      /////////////////////////
      // CSAF                //
      /////////////////////////

      /**
       * @brief Get CSAF leases by lessor
       * @param from UID of the lessor
       * @param lower_bound_to Lower bound of lessee UID to retrieve
       * @param limit Maximum number of results to return -- must not exceed 1000
       * @return Leases of same lessor, starts from a lessee whose UID not less than @ref lower_bound_to
       */
      vector<csaf_lease_object> get_csaf_leases_by_from( const account_uid_type from,
                                                         const account_uid_type lower_bound_to,
                                                         const uint32_t limit )const;

      /**
       * @brief Get CSAF leases by lessee
       * @param to UID of the lessee
       * @param lower_bound_from Lower bound of lessor UID to retrieve
       * @param limit Maximum number of results to return -- must not exceed 1000
       * @return Leases of same lessee, starts from a lessor whose UID not less than @ref lower_bound_from
       */
      vector<csaf_lease_object> get_csaf_leases_by_to( const account_uid_type to,
                                                       const account_uid_type lower_bound_from,
                                                       const uint32_t limit )const;


      /////////////////////////
      // Platforms and posts //
      /////////////////////////

      /**
       * @brief Get a list of platforms by account UID
       * @param account_uids account UIDs of the platforms to retrieve
       * @return The platforms corresponding to the provided account UIDs
       */
      vector<optional<platform_object>> get_platforms(const vector<account_uid_type>& account_uids)const;

      /**
       * @brief Get the platform owned by a given account
       * @param account The UID of the account whose platform should be retrieved
       * @return The platform object, or null if the account does not have a platform
       */
      fc::optional<platform_object> get_platform_by_account(account_uid_type account)const;

      /**
       * @brief Query for registered platforms
       * @param lower_bound_uid Lower bound of the first uid to return
       * @param limit Maximum number of results to return -- must not exceed 101
       * @param order_by how the returned list will be sorted
       * @return A list of platform objects
       */
      vector<platform_object> lookup_platforms(const account_uid_type lower_bound_uid,
                                              uint32_t limit, data_sorting_type order_by)const;

      /**
       * @brief Get the total number of platforms registered with the blockchain
       */
      uint64_t get_platform_count()const;

      /**
       * @brief Get a post
       * @param platform_owner uid of the platform
       * @param poster_uid UID of the poster
       * @param post_pid pid of the post
       * @return The post corresponding to the provided parameters.
       */
      optional<post_object> get_post( const account_uid_type platform_owner,
                                      const account_uid_type poster_uid,
                                      const post_pid_type post_pid )const;

      /**
       * @brief Get posts by platform plus poster
       * @param platform_owner uid of a platform
       * @param poster UID of a poster, will query for all posters if omitted
       * @param create_time_range a time range (earliest, latest] to query
       * @param limit Maximum number of posts to fetch (must not exceed 100)
       * @return posts corresponding to the provided parameters, ordered by create time, newest first.
       */
      // TODO may need a flag to fetch root posts only, or non-root posts only, or both
      // FIXME if limit is 100, will be buggy when too many posts in same second
      vector<post_object> get_posts_by_platform_poster( const account_uid_type platform_owner,
                                      const optional<account_uid_type> poster,
                                      const std::pair<time_point_sec, time_point_sec> create_time_range,
                                      const uint32_t limit )const;

      ////////////
      // Assets //
      ////////////

      /**
       * @brief Get a list of assets by AID
       * @param asset_ids IDs of the assets to retrieve
       * @return The assets corresponding to the provided AIDs
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<asset_object_with_data>> get_assets(const vector<asset_aid_type>& asset_ids)const;

      /**
       * @brief Get assets alphabetically by symbol name
       * @param lower_bound_symbol Lower bound of symbol names to retrieve
       * @param limit Maximum number of assets to fetch (must not exceed 101)
       * @return The assets found
       */
      vector<asset_object_with_data> list_assets(const string& lower_bound_symbol, uint32_t limit)const;

      /**
       * @brief Get a list of assets by symbol
       * @param asset_symbols Symbols or stringified IDs of the assets to retrieve
       * @return The assets corresponding to the provided symbols or IDs
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<asset_object_with_data>> lookup_asset_symbols(const vector<string>& symbols_or_ids)const;

      ///////////////
      // Witnesses //
      ///////////////

      /**
       * @brief Get a list of witnesses by account UID
       * @param account_uids account UIDs of the witnesses to retrieve
       * @return The witnesses corresponding to the provided account UIDs
       */
      vector<optional<witness_object>> get_witnesses(const vector<account_uid_type>& account_uids)const;

      /**
       * @brief Get the witness owned by a given account
       * @param account The UID of the account whose witness should be retrieved
       * @return The witness object, or null if the account does not have a witness
       */
      fc::optional<witness_object> get_witness_by_account(account_uid_type account)const;

      /**
       * @brief Query for registered witnesses
       * @param lower_bound_uid Lower bound of the first uid to return
       * @param limit Maximum number of results to return -- must not exceed 101
       * @param order_by how the returned list will be sorted
       * @return A list of witness objects
       */
      vector<witness_object> lookup_witnesses(const account_uid_type lower_bound_uid,
                                              uint32_t limit, data_sorting_type order_by)const;

      /**
       * @brief Get the total number of witnesses registered with the blockchain
       */
      uint64_t get_witness_count()const;


      /////////////////////////////////////
      // Committee members and proposals //
      /////////////////////////////////////

      /**
       * @brief Get a list of committee_members by account UID
       * @param committee_member_uids account UIDs of the committee_members to retrieve
       * @return The committee_members corresponding to the provided account UIDs
       */
      vector<optional<committee_member_object>> get_committee_members(const vector<account_uid_type>& committee_member_uids)const;

      /**
       * @brief Get the committee_member owned by a given account
       * @param account The UID of the account whose committee_member should be retrieved
       * @return The committee_member object, or null if the account does not have a committee_member
       */
      fc::optional<committee_member_object> get_committee_member_by_account(account_uid_type account)const;

      /**
       * @brief Query for registered committee members
       * @param lower_bound_uid Lower bound of the first uid to return
       * @param limit Maximum number of results to return -- must not exceed 101
       * @param order_by how the returned list will be sorted
       * @return A list of committee member objects
       */
      vector<committee_member_object> lookup_committee_members(const account_uid_type lower_bound_uid,
                                                               uint32_t limit, data_sorting_type order_by)const;

      /**
       * @brief Get the total number of committee members registered with the blockchain
       */
      uint64_t get_committee_member_count()const;

      /**
       * @brief Query for committee proposals
       * @return A list of committee proposals
       */
      vector<committee_proposal_object> list_committee_proposals() const;


      ////////////////////////////
      // Authority / validation //
      ////////////////////////////

      /// @brief Get a hexdump of the serialized binary form of a transaction
      std::string get_transaction_hex(const signed_transaction& trx)const;

      /**
       *  This API will take a partially signed transaction and a set of public keys that the owner has the ability to sign for
       *  and return the minimal subset of public keys that should add signatures to the transaction.
       */
      std::pair<std::pair<flat_set<public_key_type>,flat_set<public_key_type>>,flat_set<signature_type>> get_required_signatures( const signed_transaction& trx, const flat_set<public_key_type>& available_keys )const;

      /**
       *  This method will return the set of all public keys that could possibly sign for a given transaction.  This call can
       *  be used by wallets to filter their set of public keys to just the relevant subset prior to calling @ref get_required_signatures
       *  to get the minimum subset.
       */
      set<public_key_type> get_potential_signatures( const signed_transaction& trx )const;

      /**
       * @return true of the @ref trx has all of the required signatures, otherwise throws an exception
       */
      bool           verify_authority( const signed_transaction& trx )const;

      /**
       * @return true if the signers have enough authority to authorize an account
       */
      bool           verify_account_authority( const string& name_or_id, const flat_set<public_key_type>& signers )const;

      /**
       *  Validates a transaction against the current state without broadcasting it on the network.
       */
      processed_transaction validate_transaction( const signed_transaction& trx )const;

      /**
       *  For each operation calculate the required fee in the specified asset type.  If the asset type does
       *  not have a valid core_exchange_rate
       */
      vector< fc::variant > get_required_fees( const vector<operation>& ops, asset_id_type id )const;

      /**
       *  For each operation calculate the required fee data: payer, minimum total fee required, minimum real fee required.
       */
      vector< required_fee_data > get_required_fee_data( const vector<operation>& ops )const;

      ///////////////////////////
      // Proposed transactions //
      ///////////////////////////

      /**
       *  @return the set of proposed transactions relevant to the specified account id.
       */
      vector<proposal_object> get_proposed_transactions( account_uid_type uid )const;

   private:
      std::shared_ptr< database_api_impl > my;
};

} }

FC_REFLECT( graphene::app::required_fee_data, (fee_payer_uid)(min_fee)(min_real_fee) );

FC_REFLECT( graphene::app::full_account_query_options,
            (fetch_account_object)
            (fetch_statistics)
            (fetch_csaf_leases_in)
            (fetch_csaf_leases_out)
            (fetch_voter_object)
            (fetch_witness_object)
            (fetch_witness_votes)
            (fetch_committee_member_object)
            (fetch_committee_member_votes)
            (fetch_assets)
            (fetch_balances)
          );

FC_REFLECT_ENUM( graphene::app::data_sorting_type,
                 (order_by_uid)
                 (order_by_votes)
                 (order_by_pledge)
               );

FC_REFLECT_DERIVED( graphene::app::signed_block_with_info, (graphene::chain::signed_block),
   (block_id)(signing_key)(transaction_ids) )

FC_REFLECT_DERIVED( graphene::app::asset_object_with_data, (graphene::chain::asset_object),
   (dynamic_asset_data) )

FC_API( graphene::app::database_api,
   // Objects
   (get_objects)

   // Subscriptions
   (set_subscribe_callback)
   (set_pending_transaction_callback)
   (set_block_applied_callback)
   (cancel_all_subscriptions)

   // Blocks and transactions
   (get_block_header)
   (get_block_header_batch)
   (get_block)
   (get_transaction)
   (get_recent_transaction_by_id)

   // Globals
   (get_chain_properties)
   (get_global_properties)
   (get_config)
   (get_chain_id)
   (get_dynamic_global_properties)

   // Keys
   (get_key_references)
   (is_public_key_registered)

   // Accounts
   //(get_accounts)
   (get_accounts_by_uid)
   //(get_full_accounts)
   (get_full_accounts_by_uid)
   (get_account_by_name)
   (get_account_references)
   //(lookup_account_names)
   (lookup_accounts_by_name)
   (get_account_count)

   // CSAF
   (get_csaf_leases_by_from)
   (get_csaf_leases_by_to)

   // Platforms and posts
   (get_platforms)
   (get_platform_by_account)
   (lookup_platforms)
   (get_platform_count)
   (get_post)
   (get_posts_by_platform_poster)

   // Balances
   (get_account_balances)
   (get_named_account_balances)

   // Assets
   (get_assets)
   (list_assets)
   (lookup_asset_symbols)

   // Witnesses
   (get_witnesses)
   (get_witness_by_account)
   (lookup_witnesses)
   (get_witness_count)

   // Committee members
   (get_committee_members)
   (get_committee_member_by_account)
   (lookup_committee_members)
   (get_committee_member_count)
   (list_committee_proposals)

   // Authority / validation
   (get_transaction_hex)
   (get_required_signatures)
   //(get_potential_signatures)
   //(verify_authority)
   //(verify_account_authority)
   //(validate_transaction)
   //(get_required_fees)
   (get_required_fee_data)

   // Proposed transactions
   (get_proposed_transactions)

)
