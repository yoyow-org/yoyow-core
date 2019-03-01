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

#include <graphene/app/api.hpp>
#include <graphene/utilities/key_conversion.hpp>

using namespace graphene::app;
using namespace graphene::chain;
using namespace graphene::utilities;
using namespace std;

namespace fc
 {
   void to_variant( const account_multi_index_type& accts, variant& vo, uint32_t max_depth );
   void from_variant( const variant &var, account_multi_index_type &vo, uint32_t max_depth );
 }

namespace graphene { namespace wallet {

typedef uint16_t transaction_handle_type;

/**
 * This class takes a variant and turns it into an object
 * of the given type, with the new operator.
 */

object* create_object( const variant& v );

struct plain_keys
{
   map<public_key_type, string>  keys;
   fc::sha512                    checksum;
};

struct brain_key_info
{
   string brain_priv_key;
   string wif_priv_key;
   public_key_type pub_key;
};

// account
typedef multi_index_container<
   account_object,
   indexed_by<
      ordered_unique< tag<by_name>, member<account_object, string, &account_object::name> >,
      ordered_unique< tag<by_uid>, member<account_object, account_uid_type, &account_object::uid> >
   >
> wallet_account_multi_index_type;


struct key_label
{
   string          label;
   public_key_type key;
};


struct by_label;
struct by_key;
typedef multi_index_container<
   key_label,
   indexed_by<
      ordered_unique< tag<by_label>, member< key_label, string, &key_label::label > >,
      ordered_unique< tag<by_key>, member< key_label, public_key_type, &key_label::key > >
   >
> key_label_index_type;


struct wallet_data
{
   /** Chain ID this wallet is used with */
   chain_id_type chain_id;
   wallet_account_multi_index_type my_accounts;
   /// @return IDs of all accounts in @ref my_accounts
   vector<object_id_type> my_account_ids()const
   {
      vector<object_id_type> ids;
      ids.reserve(my_accounts.size());
      std::transform(my_accounts.begin(), my_accounts.end(), std::back_inserter(ids),
                     [](const account_object& ao) { return ao.id; });
      return ids;
   }
   /// @return UIDs of all accounts in @ref my_accounts
   vector<account_uid_type> my_account_uids()const
   {
      vector<account_uid_type> uids;
      uids.reserve(my_accounts.size());
      std::transform(my_accounts.begin(), my_accounts.end(), std::back_inserter(uids),
                     [](const account_object& ao) { return ao.uid; });
      return uids;
   }
   /// Add acct to @ref my_accounts, or update it if it is already in @ref my_accounts
   /// @return true if the account was newly inserted; false if it was only updated
   bool update_account(const account_object& acct)
   {
      auto& idx = my_accounts.get<by_uid>();
      auto itr = idx.find(acct.get_uid());
      if( itr != idx.end() )
      {
         idx.replace(itr, acct);
         return false;
      } else {
         idx.insert(acct);
         return true;
      }
   }

   /** encrypted keys */
   vector<char>              cipher_keys;

   /** map an account to a set of extra keys that have been imported for that account */
   map<account_uid_type, set<public_key_type> >  extra_keys;

   // map of account_name -> base58_private_key for
   //    incomplete account regs
   map<string, vector<string> > pending_account_registrations;
   map<string, string> pending_witness_registrations;

   key_label_index_type                                              labeled_keys;

   string                    ws_server = "ws://localhost:8090";
   string                    ws_user;
   string                    ws_password;
};

struct exported_account_keys
{
    string account_name;
    vector<vector<char>> encrypted_private_keys;
    vector<public_key_type> public_keys;
};

struct exported_keys
{
    fc::sha512 password_checksum;
    vector<exported_account_keys> account_keys;
};

struct approval_delta
{
   vector<string> secondary_approvals_to_add;
   vector<string> secondary_approvals_to_remove;
   vector<string> active_approvals_to_add;
   vector<string> active_approvals_to_remove;
   vector<string> owner_approvals_to_add;
   vector<string> owner_approvals_to_remove;
   vector<string> key_approvals_to_add;
   vector<string> key_approvals_to_remove;
};

struct post_update_ext
{
    optional<std::string>               forward_price;
    optional<std::string>               receiptor;
    optional<bool>                      to_buyout;
    optional<uint16_t>                  buyout_ratio;
    optional<std::string>               buyout_price;
    optional<time_point_sec>            buyout_expiration;
    optional<license_lid_type>          license_lid;
    optional<uint32_t>                  permission_flags;
};

struct receiptor_ext
{
    uint16_t        cur_ratio;
    bool            to_buyout;
    uint16_t        buyout_ratio;
    std::string     buyout_price;
};

struct post_create_ext
{
    uint8_t post_type = post_operation::Post_Type_Post;
    optional<std::string> forward_price;
    optional< map<account_uid_type, receiptor_ext> > receiptors;
    optional<license_lid_type> license_lid;
    uint32_t permission_flags = post_object::Post_Permission_Forward |
                                post_object::Post_Permission_Liked |
                                post_object::Post_Permission_Buyout |
                                post_object::Post_Permission_Comment |
                                post_object::Post_Permission_Reward;
};

namespace detail {
class wallet_api_impl;
}

/***
 * A utility class for performing various state-less actions that are related to wallets
 */
class utility {
   public:
      /**
       * Derive any number of *possible* owner keys from a given brain key.
       *
       * NOTE: These keys may or may not match with the owner keys of any account.
       * This function is merely intended to assist with account or key recovery.
       *
       * @see suggest_brain_key()
       *
       * @param brain_key    Brain key
       * @param number_of_desired_keys  Number of desired keys
       * @return A list of keys that are deterministically derived from the brainkey
       */
      static vector<brain_key_info> derive_owner_keys_from_brain_key(string brain_key, int number_of_desired_keys = 1);
};

struct operation_detail {
   string                   memo;
   string                   description;
   uint32_t                 sequence;
   operation_history_object op;
};

/**
 * This wallet assumes it is connected to the database server with a high-bandwidth, low-latency connection and
 * performs minimal caching. This API could be provided locally to be used by a web interface.
 */
class wallet_api
{
   public:
      wallet_api( const wallet_data& initial_data, fc::api<login_api> rapi );
      virtual ~wallet_api();

      bool copy_wallet_file( string destination_filename );

      fc::ecc::private_key derive_private_key(const std::string& prefix_string, int sequence_number) const;

      variant                           info();
      /** Returns info such as client version, git version of graphene/fc, version of boost, openssl.
       * @returns compile time info and client and dependencies versions
       */
      variant_object                    about() const;
      optional<signed_block_with_info>    get_block( uint32_t num );
      /** Returns the number of accounts registered on the blockchain
       * @returns the number of registered accounts
       */
      uint64_t                          get_account_count()const;
      /** Lists all accounts controlled by this wallet.
       * This returns a list of the account objects for all accounts whose private keys
       * we possess.
       * Note: current implementation only returns data from local cache, so it may be stale.
       *       To get latest data, a workaround is to reopen the wallet file.
       * @returns a list of account objects
       */
      vector<account_object>            list_my_accounts_cached();
      /** Lists all accounts registered in the blockchain.
       * This returns a list of all account names and their account uids, sorted by account name.
       *
       * Use the \c lowerbound and limit parameters to page through the list.  To retrieve all accounts,
       * start by setting \c lowerbound to the empty string \c "", and then each iteration, pass
       * the last account name returned as the \c lowerbound for the next \c list_accounts_by_name() call.
       *
       * @param lowerbound the name of the first account to return.  If the named account does not exist, 
       *                   the list will start at the account that comes after \c lowerbound
       * @param limit the maximum number of accounts to return (max: 1001)
       * @returns a list of accounts mapping account names to account uids
       */
      map<string,account_uid_type>       list_accounts_by_name(const string& lowerbound, uint32_t limit);
      /** List the balances of an account.
       * Each account can have multiple balances, one for each type of asset owned by that
       * account.  The returned list will only contain assets for which the account has a
       * nonzero balance
       * @param name the name or UID of the account whose balances you want
       * @returns a list of the given account's balances
       */
      vector<asset>                     list_account_balances(const string& name);
      /** Lists all assets registered on the blockchain.
       * 
       * To list all assets, pass the empty string \c "" for the lowerbound to start
       * at the beginning of the list, and iterate as necessary.
       *
       * @param lowerbound  the symbol of the first asset to include in the list.
       * @param limit the maximum number of assets to return (max: 101)
       * @returns the list of asset objects, ordered by symbol
       */
      vector<asset_object_with_data>    list_assets(const string& lowerbound, uint32_t limit)const;

      /** Returns the relative operations on the account from start number.
       *
       * @param account the name or UID of the account
       * @param op_type the operation_type to query
       * @param stop Sequence number of earliest operation.
       * @param limit the number of entries to return, should be no more than 100 if start is 0
       * @param start the sequence number where to start looping back throw the history, set 0 for most recent
       * @returns a list of \c operation_history_objects
       */
     vector<operation_detail>  get_relative_account_history(string account, optional<uint16_t> op_type, uint32_t stop, int limit, uint32_t start)const;

      /** Returns the block chain's slowly-changing settings.
       * This object contains all of the properties of the blockchain that are fixed
       * or that change only once per maintenance interval (daily) such as the
       * current list of witnesses, committee_members, block interval, etc.
       * @see \c get_dynamic_global_properties() for frequently changing properties
       * @returns the global properties
       */
      global_property_object            get_global_properties() const;
      content_parameter_extension_type  get_global_properties_extensions() const;

      /** Returns the block chain's rapidly-changing properties.
       * The returned object contains information that changes every block interval
       * such as the head block number, the next witness, etc.
       * @see \c get_global_properties() for less-frequently changing properties
       * @returns the dynamic global properties
       */
      dynamic_global_property_object    get_dynamic_global_properties() const;

      /** Returns information about the given account.
       *
       * @param account_name_or_id the name or id of the account to provide information about
       * @returns the public account data stored in the blockchain
       */
      account_object                    get_account(string account_name_or_id) const;

      /** Returns information about the given account.
       *
       * @param account_name_or_uid the name or uid of the account to provide information about
       * @returns the public account data stored in the blockchain
       */
      full_account                      get_full_account(string account_name_or_uid) const;

      /** Returns information about the given asset.
       * @param asset_name_or_id the symbol or id of the asset in question
       * @returns the information about the asset stored in the block chain
       */
      asset_object_with_data            get_asset(string asset_name_or_id) const;

      /**
       * Lookup the id of a named asset.
       * @param asset_name_or_id the symbol of an asset to look up
       * @returns the id of the given asset
       */
      asset_aid_type                    get_asset_aid(string asset_name_or_id) const;

      /**
       * Returns the blockchain object corresponding to the given id.
       *
       * This generic function can be used to retrieve any object from the blockchain
       * that is assigned an ID.  Certain types of objects have specialized convenience 
       * functions to return their objects -- e.g., assets have \c get_asset(), accounts 
       * have \c get_account(), but this function will work for any object.
       *
       * @param id the id of the object to return
       * @returns the requested object
       */
      variant                           get_object(object_id_type id) const;

      /** Returns the current wallet filename.  
       *
       * This is the filename that will be used when automatically saving the wallet.
       *
       * @see set_wallet_filename()
       * @return the wallet filename
       */
      string                            get_wallet_filename() const;

      /**
       * Get the WIF private key corresponding to a public key.  The
       * private key must already be in the wallet.
       */
      string                            get_private_key( public_key_type pubkey )const;

      /**
       * @ingroup Transaction Builder API
       */
      transaction_handle_type begin_builder_transaction();
      /**
       * @ingroup Transaction Builder API
       */
      void add_operation_to_builder_transaction(transaction_handle_type transaction_handle, const operation& op);
      /**
       * @ingroup Transaction Builder API
       */
      void replace_operation_in_builder_transaction(transaction_handle_type handle,
                                                    unsigned operation_index,
                                                    const operation& new_op);
      /**
       * @ingroup Transaction Builder API
       */
      asset set_fees_on_builder_transaction(transaction_handle_type handle, string fee_asset = GRAPHENE_SYMBOL);
      /**
       * @ingroup Transaction Builder API
       */
      transaction preview_builder_transaction(transaction_handle_type handle);
      /**
       * @ingroup Transaction Builder API
       */
      signed_transaction sign_builder_transaction(transaction_handle_type transaction_handle, bool broadcast = true);
      /**
       * @ingroup Transaction Builder API
       */
      signed_transaction propose_builder_transaction(
         transaction_handle_type handle,
         string account_name_or_id,
         time_point_sec expiration = time_point::now() + fc::minutes(1),
         uint32_t review_period_seconds = 0,
         bool broadcast = true
        );

        /** Approve or disapprove a proposal.
       *
       * @param fee_paying_account The account paying the fee for the op.
       * @param proposal_id The proposal to modify.
       * @param delta Members contain approvals to create or remove.  In JSON you can leave empty members undefined.
       * @param broadcast true if you wish to broadcast the transaction
       * @return the signed version of the transaction
       */
      signed_transaction approve_proposal(
         const string& fee_paying_account,
         const string& proposal_id,
         const approval_delta& delta,
         bool  csaf_fee,
         bool broadcast /* = false */
         );

      vector<proposal_object> list_proposals( string account_name_or_id );

      /**
       * @ingroup Transaction Builder API
       */
      void remove_builder_transaction(transaction_handle_type handle);

      /** Checks whether the wallet has just been created and has not yet had a password set.
       *
       * Calling \c set_password will transition the wallet to the locked state.
       * @return true if the wallet is new
       * @ingroup Wallet Management
       */
      bool    is_new()const;
      
      /** Checks whether the wallet is locked (is unable to use its private keys).  
       *
       * This state can be changed by calling \c lock() or \c unlock().
       * @return true if the wallet is locked
       * @ingroup Wallet Management
       */
      bool    is_locked()const;
      
      /** Locks the wallet immediately.
       * @ingroup Wallet Management
       */
      void    lock();
      
      /** Unlocks the wallet.  
       *
       * The wallet remain unlocked until the \c lock is called
       * or the program exits.
       * @param password the password previously set with \c set_password()
       * @ingroup Wallet Management
       */
      void    unlock(string password);
      
      /** Sets a new password on the wallet.
       *
       * The wallet must be either 'new' or 'unlocked' to
       * execute this command.
       * @ingroup Wallet Management
       */
      void    set_password(string password);

      /** Dumps all private keys owned by the wallet.
       *
       * The keys are printed in WIF format.  You can import these keys into another wallet
       * using \c import_key()
       * @returns a map containing the private keys, indexed by their public key 
       */
      map<public_key_type, string> dump_private_keys();

      /** Returns a list of all commands supported by the wallet API.
       *
       * This lists each command, along with its arguments and return types.
       * For more detailed help on a single command, use \c get_help()
       *
       * @returns a multi-line string suitable for displaying on a terminal
       */
      string  help()const;

      /** Returns detailed help on a single API command.
       * @param method the name of the API command you want help with
       * @returns a multi-line string suitable for displaying on a terminal
       */
      string  gethelp(const string& method)const;

      /** Loads a specified Graphene wallet.
       *
       * The current wallet is closed before the new wallet is loaded.
       *
       * @warning This does not change the filename that will be used for future
       * wallet writes, so this may cause you to overwrite your original
       * wallet unless you also call \c set_wallet_filename()
       *
       * @param wallet_filename the filename of the wallet JSON file to load.
       *                        If \c wallet_filename is empty, it reloads the
       *                        existing wallet file
       * @returns true if the specified wallet is loaded
       */
      bool    load_wallet_file(string wallet_filename = "");

      /** Saves the current wallet to the given filename.
       * 
       * @warning This does not change the wallet filename that will be used for future
       * writes, so think of this function as 'Save a Copy As...' instead of
       * 'Save As...'.  Use \c set_wallet_filename() to make the filename
       * persist.
       * @param wallet_filename the filename of the new wallet JSON file to create
       *                        or overwrite.  If \c wallet_filename is empty,
       *                        save to the current filename.
       */
      void    save_wallet_file(string wallet_filename = "");

      /** Sets the wallet filename used for future writes.
       *
       * This does not trigger a save, it only changes the default filename
       * that will be used the next time a save is triggered.
       *
       * @param wallet_filename the new filename to use for future saves
       */
      void    set_wallet_filename(string wallet_filename);

      /** Suggests a safe brain key to use for creating your account.
       * \c create_account_with_brain_key() requires you to specify a 'brain key',
       * a long passphrase that provides enough entropy to generate cyrptographic
       * keys.  This function will suggest a suitably random string that should
       * be easy to write down (and, with effort, memorize).
       * @returns a suggested brain_key
       */
      brain_key_info suggest_brain_key()const;

     /**
      * Calculate account uid from a given number.
      * @param n a number
      * @return an account uid
      */
     uint64_t calculate_account_uid(uint64_t n)const;

     /**
      * Derive any number of *possible* owner keys from a given brain key.
      *
      * NOTE: These keys may or may not match with the owner keys of any account.
      * This function is merely intended to assist with account or key recovery.
      *
      * @see suggest_brain_key()
      *
      * @param brain_key    Brain key
      * @param number_of_desired_keys  Number of desired keys
      * @return A list of keys that are deterministically derived from the brainkey
      */
     vector<brain_key_info> derive_owner_keys_from_brain_key(string brain_key, int number_of_desired_keys = 1) const;

     /**
      * Determine whether a textual representation of a public key
      * (in Base-58 format) is *currently* linked
      * to any *registered* (i.e. non-stealth) account on the blockchain
      * @param public_key Public key
      * @return Whether a public key is known
      */
     bool is_public_key_registered(string public_key) const;

      /** Converts a signed_transaction in JSON form to its binary representation.
       *
       * TODO: I don't see a broadcast_transaction() function, do we need one?
       *
       * @param tx the transaction to serialize
       * @returns the binary form of the transaction.  It will not be hex encoded, 
       *          this returns a raw string that may have null characters embedded 
       *          in it
       */
      string serialize_transaction(signed_transaction tx) const;

      /** Imports the private key for an existing account.
       *
       * The private key may match either an owner key or an active key
       * or a secondary key or the memo key for the named account.  
       *
       * @see dump_private_keys()
       *
       * @param account_name_or_id the account owning the key
       * @param wif_key the private key in WIF format
       * @returns true if the key imported matches one of the key(s) for the account
       */
      bool import_key(string account_name_or_id, string wif_key);

      map<string, bool> import_accounts( string filename, string password );

      bool import_account_keys( string filename, string password, string src_account_name, string dest_account_name );

      /** Transforms a brain key to reduce the chance of errors when re-entering the key from memory.
       *
       * This takes a user-supplied brain key and normalizes it into the form used
       * for generating private keys.  In particular, this upper-cases all ASCII characters
       * and collapses multiple spaces into one.
       * @param s the brain key as supplied by the user
       * @returns the brain key in its normalized form
       */
      string normalize_brain_key(string s) const;

      /** Registers a third party's account on the blockckain.
       *
       * This function is used to register an account for which you do not own the private keys.
       * When acting as a registrar, an end user will generate their own private keys and send
       * you the public keys.  The registrar will use this function to register the account
       * on behalf of the end user.
       *
       * @see create_account_with_brain_key()
       *
       * @param name the name of the account, must be unique on the blockchain.  Shorter names
       *             are more expensive to register; the rules are still in flux, but in general
       *             names of more than 8 characters with at least one digit will be cheap.
       * @param owner the owner key for the new account
       * @param active the active key for the new account
       * @param registrar_account the account which will pay the fee to register the user
       * @param referrer_account the account who is acting as a referrer, and may receive a
       *                         portion of the user's transaction fees.  This can be the
       *                         same as the registrar_account if there is no referrer.
       * @param referrer_percent the percentage (0 - 100) of the new user's transaction fees
       *                         not claimed by the blockchain that will be distributed to the
       *                         referrer; the rest will be sent to the registrar.  Will be
       *                         multiplied by GRAPHENE_1_PERCENT when constructing the transaction.
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction registering the account
       */
      signed_transaction register_account(string name,
                                          public_key_type owner,
                                          public_key_type active,
                                          string  registrar_account,
                                          string  referrer_account,
                                          uint32_t referrer_percent,
                                          uint32_t seed,
                                          bool csaf_fee = true,
                                          bool broadcast = false);

      /** Creates a new account and registers it on the blockchain.
       *
       * @todo why no referrer_percent here?
       *
       * @see suggest_brain_key()
       * @see register_account()
       *
       * @param brain_key the brain key used for generating the account's private keys
       * @param account_name the name of the account, must be unique on the blockchain.  Shorter names
       *                     are more expensive to register; the rules are still in flux, but in general
       *                     names of more than 8 characters with at least one digit will be cheap.
       * @param registrar_account the account which will pay the fee to register the user
       * @param referrer_account the account who is acting as a referrer, and may receive a
       *                         portion of the user's transaction fees.  This can be the
       *                         same as the registrar_account if there is no referrer.
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction registering the account
       */
      signed_transaction create_account_with_brain_key(string brain_key,
                                                       string account_name,
                                                       string registrar_account,
                                                       string referrer_account,
                                                       uint32_t seed,
                                                       bool csaf_fee = true,
                                                       bool broadcast = false);

      /** Transfer an amount from one account to another.
       * @param from the name or id of the account sending the funds
       * @param to the name or id of the account receiving the funds
       * @param amount the amount to send (in nominal units -- to send half of an asset, specify 0.5)
       * @param asset_symbol the symbol or id of the asset to send
       * @param memo a memo to attach to the transaction.  The memo will be encrypted in the
       *             transaction and readable for the receiver.  There is no length limit
       *             other than the limit imposed by maximum transaction size, but transaction fee
       *             increase with transaction size
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction transferring funds
       */
      signed_transaction transfer(string from,
                                  string to,
                                  string amount,
                                  string asset_symbol,
                                  string memo,
                                  bool csaf_fee = true,
                                  bool broadcast = false);

      signed_transaction transfer_extension(string from,
                                            string to,
                                            string amount,
                                            string asset_symbol,
                                            string memo,
                                            bool isfrom_balance = true,
                                            bool isto_balance = true,
                                            bool csaf_fee = true,
                                            bool broadcast = false);

      /** Force one account to transfer an amount to another account, can only used by asset issuer.
       * @param from the name or id of the account sending the funds
       * @param to the name or id of the account receiving the funds
       * @param amount the amount to send (in nominal units -- to send half of an asset, specify 0.5)
       * @param asset_symbol the symbol or id of the asset to send
       * @param memo a memo to attach to the transaction.  The memo will be encrypted in the
       *             transaction and readable for the receiver.  There is no length limit
       *             other than the limit imposed by maximum transaction size, but transaction fee
       *             increase with transaction size
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction transferring funds
       */
      signed_transaction override_transfer(string from,
                                  string to,
                                  string amount,
                                  string asset_symbol,
                                  string memo,
                                  bool csaf_fee = true,
                                  bool broadcast = false);

      /**
       *  This method works just like transfer, except it always broadcasts and
       *  returns the transaction ID along with the signed transaction.
       */
      pair<transaction_id_type,signed_transaction> transfer2(string from,
                                                             string to,
                                                             string amount,
                                                             string asset_symbol,
                                                             string memo ) {
         auto trx = transfer( from, to, amount, asset_symbol, memo, true, true );
         return std::make_pair(trx.id(),trx);
      }


      /**
       *  This method is used to convert a JSON transaction to its transactin ID.
       */
      transaction_id_type get_transaction_id( const signed_transaction& trx )const { return trx.id(); }


      /** These methods are used for stealth transfers */
      ///@{
      /**
       *  This method can be used to set the label for a public key
       *
       *  @note No two keys can have the same label.
       *
       *  @return true if the label was set, otherwise false
       */
      bool                        set_key_label( public_key_type, string label );
      string                      get_key_label( public_key_type )const;

      /** @return the public key associated with the given label */
      public_key_type             get_public_key( string label )const;
      ///@}

      /** Creates a new user-issued asset.
       *
       * Many options can be changed later using \c update_asset()
       *
       * Must provide raw JSON data structure for the options object.
       *
       * @param issuer the name or id of the account who will pay the fee and become the 
       *               issuer of the new asset.
       * @param symbol the ticker symbol of the new asset
       * @param precision the number of digits of precision to the right of the decimal point,
       *                  must be less than or equal to 12
       * @param common asset options required for the new asset.
       * @param initial_supply issue this amount to self immediately after the asset is created.
       *                       Note: this amount is NOT in nominal units, E.G. if precision of
       *                         new asset is 3, to issue 1 nominal unit of asset, specify 1000.
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction creating a new asset
       */
      signed_transaction create_asset(string issuer,
                                      string symbol,
                                      uint8_t precision,
                                      asset_options common,
                                      share_type initial_supply,
                                      bool csaf_fee = true,
                                      bool broadcast = false);

      /** Issue new shares of an asset.
       *
       * @param to_account the name or id of the account to receive the new shares
       * @param amount the amount to issue, in nominal units
       * @param symbol the ticker symbol of the asset to issue
       * @param memo a memo to include in the transaction, readable by the recipient
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction issuing the new shares
       */
      signed_transaction issue_asset(string to_account, string amount,
                                     string symbol,
                                     string memo,
                                     bool csaf_fee = true,
                                     bool broadcast = false);

      /** Update options of an asset.
       *
       * There are a number of options which all assets in the network use. These options are 
       * enumerated in the asset_object::asset_options struct. This command is used to update 
       * these options for an existing asset.
       *
       * @param symbol the name or id of the asset to update
       * @param new_precision if changing the asset's precision, the new precision.
       *                   null if you wish to remain the precision of the asset
       * @param new_options the new asset_options object, which will entirely replace the existing
       *                    options.
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction updating the asset
       */
      signed_transaction update_asset(string symbol,
                                      optional<uint8_t> new_precision,
                                      asset_options new_options,
                                      bool csaf_fee = true,
                                      bool broadcast = false);

      /** Burns the given amount of asset.
       *
       * This command burns an amount of asset to reduce the amount in circulation.
       * @param from the account containing the asset you wish to burn
       * @param amount the amount to burn, in nominal units
       * @param symbol the name or id of the asset to burn
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction burning the asset
       */
      signed_transaction reserve_asset(string from,
                                    string amount,
                                    string symbol,
                                    bool csaf_fee = true,
                                    bool broadcast = false);

      /** Whitelist and blacklist accounts, primarily for transacting in whitelisted assets.
       *
       * Accounts can freely specify opinions about other accounts, in the form of either whitelisting or blacklisting
       * them. This information is used in chain validation only to determine whether an account is authorized to transact
       * in an asset type which enforces a whitelist, but third parties can use this information for other uses as well,
       * as long as it does not conflict with the use of whitelisted assets.
       *
       * An asset which enforces a whitelist specifies a list of accounts to maintain its whitelist, and a list of
       * accounts to maintain its blacklist. In order for a given account A to hold and transact in a whitelisted asset S,
       * A must be whitelisted by at least one of S's whitelist_authorities and blacklisted by none of S's
       * blacklist_authorities. If A receives a balance of S, and is later removed from the whitelist(s) which allowed it
       * to hold S, or added to any blacklist S specifies as authoritative, A's balance of S will be frozen until A's
       * authorization is reinstated.
       *
       * @param authorizing_account the account who is doing the whitelisting
       * @param account_to_list the account being whitelisted
       * @param new_listing_status the new whitelisting status
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction changing the whitelisting status
       */
      signed_transaction whitelist_account(string authorizing_account,
                                           string account_to_list,
                                           account_whitelist_operation::account_listing new_listing_status,
                                           bool csaf_fee = true,
                                           bool broadcast = false);

      /** Creates a committee_member object owned by the given account.
       *
       * An account can have at most one committee_member object.
       *
       * @param owner_account the name or uid of the account which is creating the committee_member
       * @param pledge_amount The amount to set as pledge.
       * @param pledge_asset_symbol The symbol of the asset to set as pledge.
       * @param url a URL to include in the committee_member record in the blockchain.  Clients may
       *            display this when showing a list of committee_members.  May be blank.
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction registering a committee_member
       */
      signed_transaction create_committee_member(string owner_account,
                                         string pledge_amount,
                                         string pledge_asset_symbol,
                                         string url, 
                                         bool csaf_fee = true,
                                         bool broadcast = false);

      /**
       * Update a committee_member object owned by the given account.
       *
       * @param committee_member_account The UID or name of the committee member's owner account.
       * @param pledge_amount The amount to set as pledge. Do not set if want to remain the same.
       * @param pledge_asset_symbol The symbol of the asset to set as pledge. Do not set if want to remain the same.
       * @param url a URL to include in the committee member record in the blockchain.  Clients may
       *            display this when showing a list of committee members.  May be blank. Do not set if want to remain the same.
       * @param broadcast true if you wish to broadcast the transaction.
       */
      signed_transaction update_committee_member(string committee_member_account,
                                        optional<string> pledge_amount,
                                        optional<string> pledge_asset_symbol,
                                        optional<string> url,
                                        bool csaf_fee = true,
                                        bool broadcast = false);

      /** Lists all witnesses registered in the blockchain. This returns a list of all witness objects,
       * sorted by specified order.  This lists witnesses whether they are currently voted in or not.
       *
       * Use the \c lowerbound and limit parameters to page through the list.  To retrieve all witnesss,
       * start by setting \c lowerbound to zero, and then each iteration, pass
       * the last witness's UID returned as the \c lowerbound for the next \c list_witnesss() call.
       *
       * @param lowerbound the UID of the first witness to return.  If the witness does not exist,
       *                   the list will start at the witness that comes after \c lowerbound
       * @param limit the maximum number of witnesss to return (max: 101)
       * @param order_by how the returned list will be sorted
       * @returns a list of witnesses
       */
      vector<witness_object> list_witnesses(const account_uid_type lowerbound, uint32_t limit, data_sorting_type order_by);

      /** Lists all committee members registered in the blockchain. This returns a list of all committee member objects,
       * sorted by specified order.  This lists committee members whether they are currently voted in or not.
       *
       * Use the \c lowerbound and limit parameters to page through the list.  To retrieve all committee members,
       * start by setting \c lowerbound to zero, and then each iteration, pass
       * the last committee member's UID returned as the \c lowerbound for the next \c list_committee_members() call.
       *
       * @param lowerbound the UID of the first committee member to return.  If the committee member does not exist,
       *                   the list will start at the committee member that comes after \c lowerbound
       * @param limit the maximum number of committee members to return (max: 101)
       * @param order_by how the returned list will be sorted
       * @returns a list of committee members
       */
      vector<committee_member_object> list_committee_members(const account_uid_type lowerbound, uint32_t limit,
                                                             data_sorting_type order_by);

      /** List all committee proposal.
       *
       * @returns a list of committee proposals
       */
      vector<committee_proposal_object> list_committee_proposals();

      /** Returns information about the given witness.
       * @param owner_account the name or id of the witness account owner, or the id of the witness
       * @returns the information about the witness stored in the block chain
       */
      witness_object get_witness(string owner_account);

      /** Returns information about the given committee_member.
       * @param owner_account the name or id of the committee_member account owner, or the id of the committee_member
       * @returns the information about the committee_member stored in the block chain
       */
      committee_member_object get_committee_member(string owner_account);

      /** Creates a witness object owned by the given account.
       *
       * An account can have at most one witness object.
       *
       * @param owner_account the name or uid of the account which is creating the witness
       * @param block_signing_key The block signing public key.
       * @param pledge_amount The amount to set as pledge.
       * @param pledge_asset_symbol The symbol of the asset to set as pledge.
       * @param url a URL to include in the witness record in the blockchain.  Clients may
       *            display this when showing a list of witnesses.  May be blank.
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction registering a witness
       */
      signed_transaction create_witness(string owner_account,
                                        public_key_type block_signing_key,
                                        string pledge_amount,
                                        string pledge_asset_symbol,
                                        string url,
                                        bool csaf_fee = true,
                                        bool broadcast = false);

      /**
       * Update a witness object owned by the given account.
       *
       * @param witness_account The UID or name of the witness's owner account.
       * @param block_signing_key The new block signing public key.  Do not set if want to remain the same.
       * @param pledge_amount The amount to set as pledge. Do not set if want to remain the same.
       * @param pledge_asset_symbol The symbol of the asset to set as pledge. Do not set if want to remain the same.
       * @param url a URL to include in the witness record in the blockchain.  Clients may
       *            display this when showing a list of witnesses.  May be blank. Do not set if want to remain the same.
       * @param broadcast true if you wish to broadcast the transaction.
       */
      signed_transaction update_witness(string witness_account,
                                        optional<public_key_type> block_signing_key,
                                        optional<string> pledge_amount,
                                        optional<string> pledge_asset_symbol,
                                        optional<string> url,
                                        bool csaf_fee = true,
                                        bool broadcast = false);


      /**
       * Collect witness pay.
       *
       * @param witness_account The UID or name of the witness's owner account.
       * @param pay_amount The amount to collect.
       * @param pay_asset_symbol The symbol of the asset to collect.
       * @param broadcast true if you wish to broadcast the transaction.
       */
      signed_transaction collect_witness_pay(string witness_account,
                                        string pay_amount,
                                        string pay_asset_symbol,
                                        bool csaf_fee = true,
                                        bool broadcast = false);

      /**
       * Collect CSAF with current time.
       *
       * @param from The UID or name of the account that will collect CSAF from.
       * @param to The UID or name of the account that will collect CSAF to.
       * @param amount The amount to collect.
       * @param asset_symbol The symbol of the asset to collect.
       * @param broadcast true if you wish to broadcast the transaction.
       */
      signed_transaction collect_csaf(string from,
                                      string to,
                                      string amount,
                                      string asset_symbol,
                                      bool csaf_fee = true,
                                      bool broadcast = false);

      /**
       * Collect CSAF with specified time.
       *
       * @param from The UID or name of the account that will collect CSAF from.
       * @param to The UID or name of the account that will collect CSAF to.
       * @param amount The amount to collect.
       * @param asset_symbol The symbol of the asset to collect.
       * @param time The time that will be used to calculate available CSAF.
       * @param broadcast true if you wish to broadcast the transaction.
       */
      signed_transaction collect_csaf_with_time(string from,
                                      string to,
                                      string amount,
                                      string asset_symbol,
                                      fc::time_point_sec time,
                                      bool csaf_fee = true,
                                      bool broadcast = false);


      /** Returns information about the given platform.
       * @param owner_account the name or id of the platform account owner, or the id of the platform
       * @returns the information about the platform stored in the block chain
       */
      platform_object get_platform(string owner_account);


      /** Lists all platforms registered in the blockchain. This returns a list of all platform objects,
       * sorted by specified order.  This lists platforms whether they are currently voted in or not.
       *
       * Use the \c lowerbound and limit parameters to page through the list.  To retrieve all platform,
       * start by setting \c lowerbound to zero, and then each iteration, pass
       * the last platform's UID returned as the \c lowerbound for the next \c list_platforms() call.
       *
       * @param lowerbound the UID of the first platform to return.  If the platform does not exist,
       *                   the list will start at the platform that comes after \c lowerbound
       * @param limit the maximum number of platform to return (max: 101)
       * @param order_by how the returned list will be sorted
       * @returns a list of platforms
       */
      vector<platform_object> list_platforms(const account_uid_type lowerbound, uint32_t limit, data_sorting_type order_by);

      /**
       * Get total number of platforms registered with the blockchain
       *
       * @returns number of platforms
       */
      uint64_t get_platform_count();

      /** Creates a platform object owned by the given account.
       *
       * An account can have at most one platform object.
       *
       * @param owner_account the name or uid of the account which is creating the platform
       * @param name The platform's name.
       * @param pledge_amount The amount to set as pledge.
       * @param pledge_asset_symbol The symbol of the asset to set as pledge.
       * @param url a URL to include in the platform record in the blockchain.  Clients may
       *            display this when showing a list of platforms.  May be blank.
       * @param extra_data a string json format,Including api_url, platform introduction
       * @param broadcast true to broadcast the transaction on the network
       * @returns the signed transaction registering a platform
       */
      signed_transaction create_platform(string owner_account,
                                        string name,
                                        string pledge_amount,
                                        string pledge_asset_symbol,
                                        string url,
                                        string extra_data = "{}",
                                        bool csaf_fee = true,
                                        bool broadcast = false);

      /**
       * Update a platform object owned by the given account.
       *
       * @param platform_account The UID or name of the platform's owner account.
       * @param name The platform's name.
       * @param pledge_amount The amount to set as pledge. Do not set if want to remain the same.
       * @param pledge_asset_symbol The symbol of the asset to set as pledge. Do not set if want to remain the same.
       * @param url a URL to include in the platform record in the blockchain.  Clients may
       *            display this when showing a list of platforms.  May be blank. Do not set if want to remain the same.
       * @param extra_data a string json format,Including api_url, platform introduction
       * @param broadcast true if you wish to broadcast the transaction.
       */
      signed_transaction update_platform(string platform_account,
                                        optional<string> name,
                                        optional<string> pledge_amount,
                                        optional<string> pledge_asset_symbol,
                                        optional<string> url,
                                        optional<string> extra_data,
                                        bool csaf_fee = true,
                                        bool broadcast = false);

      /** Update platform voting options.
       *
       * An account can publish a list of all platforms they approve of.  This
       * command allows you to add and/or remove platforms from this list.
       * Each account's vote is weighted according to the number of shares of the
       * core asset owned by that account at the time the votes are tallied.
       *
       * @note you cannot vote against a platform, you can only vote for the platform
       *       or not vote for the platform.
       *
       * @param voting_account the name or uid of the account who is voting with their shares
       * @param platforms_to_add a list of name or uid of platforms that to be voted for
       * @param platforms_to_remove a list of name or uid of platforms that to revoke votes from
       * @param broadcast true if you wish to broadcast the transaction
       * @return the signed transaction changing your vote for the given platforms
       */
      signed_transaction update_platform_votes(string voting_account,
                                          flat_set<string> platforms_to_add,
                                          flat_set<string> platforms_to_remove,
                                          bool csaf_fee = true,
                                          bool broadcast = false);

      /**
       * auth of account to platform
       * 
       * @param account the name or uid of the account
       * @param platform_owner the platform owner uid
       * @param broadcast true if you wish to broadcast the transaction
       */ 
      signed_transaction account_auth_platform(string account,
                                               string platform_owner,
                                               string memo,
                                               string limit_for_platform,
                                               uint32_t permission_flags = account_statistics_object::Platform_Permission_Forward |
                                                                           account_statistics_object::Platform_Permission_Liked |
                                                                           account_statistics_object::Platform_Permission_Buyout |
                                                                           account_statistics_object::Platform_Permission_Comment |
                                                                           account_statistics_object::Platform_Permission_Reward |
                                                                           account_statistics_object::Platform_Permission_Post,
                                               bool csaf_fee = true,
                                               bool broadcast = false);

      /**
       * cancel auth of account to platform
       * 
       * @param account the name or uid of the account
       * @param platform_owner the platform owner uid
       * @param broadcast true if you wish to broadcast the transaction
       */ 
      signed_transaction account_cancel_auth_platform(string account,
                                                      string platform_owner,
                                                      bool csaf_fee = true,
                                                      bool broadcast = false);

      /**
       * Enable or disable allowed_assets option for an account
       *
       * @param account the name or uid of the account
       * @param enable true to enable, false to disable
       * @param broadcast true if you wish to broadcast the transaction
       */
      signed_transaction enable_allowed_assets(string account,
                                          bool enable,
                                          bool csaf_fee = true,
                                          bool broadcast = false);

      /**
       * Update allowed_assets option for an account
       *
       * @param account the name or uid of the account
       * @param assets_to_add a list of id or symbol of assets to be added to allowed_assets
       * @param assets_to_remove a list of id or symbol of assets to be removed from allowed_assets
       * @param broadcast true if you wish to broadcast the transaction
       */
      signed_transaction update_allowed_assets(string account,
                                          flat_set<string> assets_to_add,
                                          flat_set<string> assets_to_remove,
                                          bool csaf_fee = true,
                                          bool broadcast = false);

      /** Update witness voting options.
       *
       * An account can publish a list of all witnesses they approve of.  This
       * command allows you to add and/or remove witnesses from this list.
       * Each account's vote is weighted according to the number of shares of the
       * core asset owned by that account at the time the votes are tallied.
       *
       * @note you cannot vote against a witness, you can only vote for the witness
       *       or not vote for the witness.
       *
       * @param voting_account the name or uid of the account who is voting with their shares
       * @param witnesses_to_add a list of name or uid of witnesses that to be voted for
       * @param witnesses_to_remove a list of name or uid of witnesses that to revoke votes from
       * @param broadcast true if you wish to broadcast the transaction
       * @return the signed transaction changing your vote for the given witnesses
       */
      signed_transaction update_witness_votes(string voting_account,
                                          flat_set<string> witnesses_to_add,
                                          flat_set<string> witnesses_to_remove,
                                          bool csaf_fee = true,
                                          bool broadcast = false);

      /** Update committee member voting options.
       *
       * An account can publish a list of all committee members they approve of.  This
       * command allows you to add and/or remove committee members from this list.
       * Each account's vote is weighted according to the number of shares of the
       * core asset owned by that account at the time the votes are tallied.
       *
       * @note you cannot vote against a committee member, you can only vote for the committee member
       *       or not vote for the committee member.
       *
       * @param voting_account the name or uid of the account who is voting with their shares
       * @param committee_members_to_add a list of name or uid of committee members that to be voted for
       * @param committee_members_to_remove a list of name or uid of committee members that to revoke votes from
       * @param broadcast true if you wish to broadcast the transaction
       * @return the signed transaction changing your vote for the given committee members
       */
      signed_transaction update_committee_member_votes(string voting_account,
                                          flat_set<string> committee_members_to_add,
                                          flat_set<string> committee_members_to_remove,
                                          bool csaf_fee = true,
                                          bool broadcast = false);

      /** Set the voting proxy for an account.
       *
       * If a user does not wish to take an active part in voting, they can choose
       * to allow another account to vote their stake.
       *
       * Setting a vote proxy will remove your previous votes from the blockchain.
       *
       * This setting can be changed at any time.
       *
       * @param account_to_modify the name or uid of the account to update
       * @param voting_account the name or uid of an account authorized to vote account_to_modify's shares,
       *                       or null to vote your own shares
       *
       * @param broadcast true if you wish to broadcast the transaction
       * @return the signed transaction changing your vote proxy settings
       */
      signed_transaction set_voting_proxy(string account_to_modify,
                                          optional<string> voting_account,
                                          bool csaf_fee = true,
                                          bool broadcast = false);
      
      /** Signs a transaction.
       *
       * Given a fully-formed transaction that is only lacking signatures, this signs
       * the transaction with the necessary keys and optionally broadcasts the transaction
       * @param tx the unsigned transaction
       * @param broadcast true if you wish to broadcast the transaction
       * @return the signed version of the transaction
       */
      signed_transaction sign_transaction(signed_transaction tx, bool broadcast = false);

      /** Returns an uninitialized object representing a given blockchain operation.
       *
       * This returns a default-initialized object of the given type; it can be used 
       * during early development of the wallet when we don't yet have custom commands for
       * creating all of the operations the blockchain supports.  
       *
       * Any operation the blockchain supports can be created using the transaction builder's
       * \c add_operation_to_builder_transaction() , but to do that from the CLI you need to 
       * know what the JSON form of the operation looks like.  This will give you a template
       * you can fill in.  It's better than nothing.
       * 
       * @param operation_type the type of operation to return, must be one of the 
       *                       operations defined in `graphene/chain/operations.hpp`
       *                       (e.g., "global_parameters_update_operation")
       * @return a default-constructed operation of the given type
       */
      operation get_prototype_operation(string operation_type);

      /** create a committee proposal
       * 
       * @param committee_member_account The committee member
       * @param items @see committee_proposal_item_type
       * @param voting_closing_block_num When will voting for the proposal be closed
       * @param proposer_opinion Whether the proposer agree with the proposal
       * @param execution_block_num When will the proposal be executed if approved
       * @param expiration_block_num When will the proposal be tried to executed again if failed on first execution, will be ignored if failed again.
       * @param broadcast true if you wish to broadcast the transaction
       * @return the signed version of the transaction
       */ 
      signed_transaction committee_proposal_create(
         const string committee_member_account,
         const vector<committee_proposal_item_type> items,
         const uint32_t voting_closing_block_num,
         optional<voting_opinion_type> proposer_opinion,
         const uint32_t execution_block_num = 0,
         const uint32_t expiration_block_num = 0,
         bool csaf_fee = true,
         bool broadcast = false
      );

      /** Vote on a committee proposal.
       *
       * @param committee_member_account The committee member that casted the vote.
       * @param proposal_number The proposal to vote.
       * @param opinion The voter's opinion.
       * @param broadcast true if you wish to broadcast the transaction
       * @return the signed version of the transaction
       */
      signed_transaction committee_proposal_vote(
         const string committee_member_account,
         const uint64_t proposal_number,
         const voting_opinion_type opinion,
         bool csaf_fee = true,
         bool broadcast = false
         );

      signed_transaction proposal_create(const string              fee_paying_account,
                                         const vector<op_wrapper>  proposed_ops,
                                         time_point_sec            expiration_time,
                                         uint32_t                  review_period_seconds,
                                         bool                      csaf_fee = true,
                                         bool                      broadcast = false
                                         );

      signed_transaction proposal_update(const string                      fee_paying_account,
                                         proposal_id_type                  proposal,
                                         const flat_set<account_uid_type>  secondary_approvals_to_add,
                                         const flat_set<account_uid_type>  secondary_approvals_to_remove,
                                         const flat_set<account_uid_type>  active_approvals_to_add,
                                         const flat_set<account_uid_type>  active_approvals_to_remove,
                                         const flat_set<account_uid_type>  owner_approvals_to_add,
                                         const flat_set<account_uid_type>  owner_approvals_to_remove,
                                         const flat_set<public_key_type>   key_approvals_to_add,
                                         const flat_set<public_key_type>   key_approvals_to_remove,
                                         bool csaf_fee = true,
                                         bool broadcast = false
                                         );

      signed_transaction proposal_delete(const string              fee_paying_account,
                                         proposal_id_type          proposal,
                                         bool                      csaf_fee = true,
                                         bool                      broadcast = false
                                         );


      /** Creates a score object for a post object by from_account.
      *
      * An account can create only one score_object for a post_object.
      *
      * @param from_account the name or uid of the account which is creating the score_object
      * @param platform The post`s platform account.
      * @param poster The post`s publisher account.
      * @param post_pid_type The post`s id.
      * @param score given score num, from -5 to 5
      * @param csaf given some csaf from from_account for this score object 
      * @param broadcast true to broadcast the transaction on the network
      * @returns the signed transaction 
      */
      signed_transaction score_a_post(string           from_account,
                                      string           platform,
                                      string           poster,
                                      post_pid_type    post_pid,
                                      int8_t           score,
                                      string           csaf,
                                      bool csaf_fee = true,
                                      bool broadcast = false);

      /** Reward a post by from_account.
      *
      * Any asset can reward a post_object by from_account.
      *
      * @param from_account the name or uid of the account which reward the post
      * @param platform The post`s platform account.
      * @param poster The post`s publisher account.
      * @param post_pid_type The post`s id.
      * @param amount the reward asset`s amount
      * @param asset_symbol the reward asset`s symbol
      * @param broadcast true to broadcast the transaction on the network
      * @returns the signed transaction
      */
      signed_transaction reward_post(string           from_account,
                                     string           platform,
                                     string           poster,
                                     post_pid_type    post_pid,
                                     string           amount,
                                     string           asset_symbol,
                                     bool csaf_fee = true,
                                     bool broadcast = false);

      /** Reward a post by platform.
      *
      * Only pre_paid can reward a post_object by platform which allowed by from_account.
      *
      * @param from_account the name or uid of the account which reward the post
      * @param platform The post`s platform account.
      * @param poster The post`s publisher account.
      * @param post_pid_type The post`s id.
      * @param amount the reward asset`s amount, just pre_paid
      * @param broadcast true to broadcast the transaction on the network
      * @returns the signed transaction
      */
      signed_transaction reward_post_proxy_by_platform(string           from_account,
                                                       string           platform,
                                                       string           poster,
                                                       post_pid_type    post_pid,
                                                       string           amount,
                                                       bool csaf_fee = true,
                                                       bool broadcast = false);

      /** buyout a post by from_account.
      *
      * Only from_account buyout receipt ratio owned by receiptor_account.
      *
      * @param from_account the name or uid of the account which buyout the post
      * @param platform The post`s platform account.
      * @param poster The post`s publisher account.
      * @param post_pid_type The post`s id.
      * @param receiptor_account the receipt ratio`s owner
      * @param broadcast true to broadcast the transaction on the network
      * @returns the signed transaction
      */
      signed_transaction buyout_post(string           from_account,
                                     string           platform,
                                     string           poster,
                                     post_pid_type    post_pid,
                                     string           receiptor_account,
                                     bool csaf_fee = true,
                                     bool broadcast = false);

      /** create a license by platform.
      *
      * Only platformcan create a license.
      *
      * @param platform the name or uid of the platform which create this license.
      * @param license_type The license`s type.
      * @param hash_value the hash for this license.
      * @param title the license`s title
      * @param body the license`s body
      * @param extra_data the license`s extra_data
      * @param broadcast true to broadcast the transaction on the network
      * @returns the signed transaction
      */
      signed_transaction create_license(string           platform,
                                        uint8_t          license_type,
                                        string           hash_value,
                                        string           title,
                                        string           body,
                                        string           extra_data,
                                        bool csaf_fee = true,
                                        bool broadcast = false);

      /** create a post by poster and platform.
      *
      * Only platform and poster can create a post.
      *
      * @param platform the name or uid of the platform which create this license.
      * @param poster the poster.
      * @param hash_value the hash for this post.
      * @param title the post`s title.
      * @param body the post`s body.
      * @param extra_data the post`s extra_data.
      * @param origin_platform the origin post platform od the post.
      * @param origin_poster the origin poster of the post.
      * @param origin_post_pid the origin post id of the post.
      * @param ext the post`s extensions.
      * @param broadcast true to broadcast the transaction on the network.
      * @returns the signed transaction.
      */
      signed_transaction create_post(string           platform,
                                     string           poster,
                                     string           hash_value,
                                     string           title,
                                     string           body,
                                     string           extra_data,
                                     string           origin_platform = "",
                                     string           origin_poster = "",
                                     string           origin_post_pid = "",
                                     post_create_ext  ext = post_create_ext(),
                                     bool csaf_fee = true,
                                     bool broadcast = false);

      /** update a post by poster and platform.
      *
      * Only platform and poster can update a post.
      *
      * @param platform the name or uid of the platform which create this license.
      * @param poster the poster.
      * @param post_pid the poster`s id.
      * @param hash_value the hash to update.
      * @param title the title to update.
      * @param body the body to update.
      * @param extra_data the extra_data to update.
      * @param ext the post`s extensions.
      * @param broadcast true to broadcast the transaction on the network.
      * @returns the signed transaction.
      */
      signed_transaction update_post(string           platform,
                                     string           poster,
                                     string           post_pid,
                                     string           hash_value = "",
                                     string           title = "",
                                     string           body = "",
                                     string           extra_data = "",
                                     post_update_ext ext = post_update_ext(),
                                     bool csaf_fee = true,
                                     bool broadcast = false);

      signed_transaction account_manage(string executor,
                                        string account,
                                        account_manage_operation::opt options,
                                        bool csaf_fee = true,
                                        bool broadcast = false
                                        );

      signed_transaction buy_advertising(string               account,
                                         string               platform,
                                         object_id_type       advertising_id,
                                         uint32_t             start_time,
                                         uint32_t             buy_number,
                                         string               extra_data,
                                         string               memo,
                                         bool                 csaf_fee = true,
                                         bool                 broadcast = false
                                        );

      signed_transaction confirm_advertising(string         platform,
                                             object_id_type advertising_id,
                                             uint32_t       order_sequence,
                                             bool           comfirm,
                                             bool           csaf_fee = true,
                                             bool           broadcast = false
                                             );

      post_object get_post( string platform_owner,
                            string poster_uid,
                            string post_pid );

      vector<post_object> get_posts_by_platform_poster(string                                    platform_owner,
                                                       string                                    poster,
                                                       uint32_t                                  begin_time_range,
                                                       uint32_t                                  end_time_range,
                                                       uint32_t                                  limit);

      score_object get_score(string platform,
                             string poster_uid,
                             string post_pid,
                             string from_account);

      vector<score_object> list_scores(string   platform,
                                       string   poster_uid,
                                       string   post_pid,
                                       uint32_t limit,
                                       bool     list_cur_period = true);

      license_object get_license(string platform,
                                 string license_lid);

      vector<license_object> list_licenses(string platform, uint32_t limit);

      advertising_object get_advertising(object_id_type advertising_id);
                      
      vector<advertising_object> list_advertisings(string platform, uint32_t limit = 100);

      vector<active_post_object> get_post_profits_detail(uint32_t         begin_period,
                                                         uint32_t         end_period,
                                                         string           platform,
                                                         string           poster,
                                                         string           post_pid);

      vector<Platform_Period_Profit_Detail> get_platform_profits_detail(uint32_t         begin_period,
                                                                        uint32_t         end_period,
                                                                        string           platform);

      vector<Poster_Period_Profit_Detail> get_poster_profits_detail(uint32_t         begin_period,
                                                                    uint32_t         end_period,
                                                                    string           poster);

      share_type get_score_profit(string account, uint32_t period);

      account_statistics_object get_account_statistics(string account);

      signed_transaction create_advertising(string           platform,
                                            string           description,
                                            string           unit_price,
                                            uint32_t         unit_time,
                                            bool csaf_fee = true,
                                            bool broadcast = false);

      signed_transaction update_advertising(string                     platform,
                                            object_id_type             advertising_id,
                                            optional<string>           description,
                                            optional<string>           unit_price,
                                            optional<uint32_t>         unit_time,
                                            optional<bool>             on_sell,
                                            bool csaf_fee = true,
                                            bool broadcast = false);

      signed_transaction ransom_advertising(string           platform,
                                            string           from_account,
                                            object_id_type   advertising_id,
                                            uint32_t         order_sequence,
                                            bool csaf_fee = true,
                                            bool broadcast = false);
         
      void dbg_make_uia(string creator, string symbol);
      void dbg_push_blocks( std::string src_filename, uint32_t count );
      void dbg_generate_blocks( std::string debug_wif_key, uint32_t count );
      void dbg_stream_json_objects( const std::string& filename );
      void dbg_update_object( fc::variant_object update );

      void flood_network(string prefix, uint32_t number_of_transactions);

      void network_add_nodes( const vector<string>& nodes );
      vector< variant > network_get_connected_peers();

      std::map<string,std::function<string(fc::variant,const fc::variants&)>> get_result_formatters() const;

      fc::signal<void(bool)> lock_changed;
      std::shared_ptr<detail::wallet_api_impl> my;
      void encrypt_keys();
};

} }

namespace fc
{
   void to_variant(const graphene::wallet::wallet_account_multi_index_type& accts, variant& vo);
   void from_variant(const variant &var, graphene::wallet::wallet_account_multi_index_type &vo);
}

FC_REFLECT( graphene::wallet::key_label, (label)(key) )

FC_REFLECT( graphene::wallet::plain_keys, (keys)(checksum) )

FC_REFLECT( graphene::wallet::wallet_data,
            (chain_id)
            (my_accounts)
            (cipher_keys)
            (extra_keys)
            (pending_account_registrations)(pending_witness_registrations)
            (labeled_keys)
            (ws_server)
            (ws_user)
            (ws_password)
          )

FC_REFLECT( graphene::wallet::brain_key_info,
            (brain_priv_key)
            (wif_priv_key)
            (pub_key)
          )

FC_REFLECT( graphene::wallet::exported_account_keys, (account_name)(encrypted_private_keys)(public_keys) )

FC_REFLECT( graphene::wallet::exported_keys, (password_checksum)(account_keys) )

FC_REFLECT( graphene::wallet::approval_delta,
   (secondary_approvals_to_add)
   (secondary_approvals_to_remove)
   (active_approvals_to_add)
   (active_approvals_to_remove)
   (owner_approvals_to_add)
   (owner_approvals_to_remove)
   (key_approvals_to_add)
   (key_approvals_to_remove)
)

FC_REFLECT(graphene::wallet::post_update_ext,
          (forward_price)
          (receiptor)
          (to_buyout)
          (buyout_ratio)
          (buyout_price)
          (license_lid)
          (permission_flags)
          )

FC_REFLECT(graphene::wallet::receiptor_ext,
          (cur_ratio)
          (to_buyout)
          (buyout_ratio)
          (buyout_price)
          )

FC_REFLECT(graphene::wallet::post_create_ext,
          (post_type)
          (forward_price)
          (receiptors)
          (license_lid)
          (permission_flags)
          )

FC_REFLECT( graphene::wallet::operation_detail, 
            (memo)(description)(sequence)(op) )

FC_API( graphene::wallet::wallet_api,
        (help)
        (gethelp)
        (info)
        (about)
        //(begin_builder_transaction)
        //(add_operation_to_builder_transaction)
        //(replace_operation_in_builder_transaction)
        //(set_fees_on_builder_transaction)
        //(preview_builder_transaction)
        //(sign_builder_transaction)
        //(propose_builder_transaction)
        //(remove_builder_transaction)
        (approve_proposal)
        //(list_proposals)
        (is_new)
        (is_locked)
        (lock)(unlock)(set_password)
        (dump_private_keys)
        (list_my_accounts_cached)
        (list_accounts_by_name)
        (list_account_balances)
        (list_assets)
        (import_key)
        //(import_accounts)
        //(import_account_keys)
        (suggest_brain_key)
        (calculate_account_uid)
        //(derive_owner_keys_from_brain_key)
        (register_account)
        (create_account_with_brain_key)
        (transfer)
        (transfer_extension)
        (override_transfer)
        //(transfer2)
        (get_transaction_id)
        (create_asset)
        (update_asset)
        (issue_asset)
        (get_asset)
        (reserve_asset)
        (enable_allowed_assets)
        (update_allowed_assets)
        //(whitelist_account)
        (create_committee_member)
        (update_committee_member)
        (get_committee_member)
        (list_committee_members)
        (update_committee_member_votes)
        (list_committee_proposals)
        (committee_proposal_create)
        (committee_proposal_vote)
        (proposal_create)
        (proposal_update)
        (proposal_delete)
        (create_witness)
        (update_witness)
        (get_witness)
        (list_witnesses)
        (update_witness_votes)
        (collect_witness_pay)
        (collect_csaf)
        (collect_csaf_with_time)
        (get_platform)
        (list_platforms)
        (get_platform_count)
        (create_platform)
        (update_platform)
        (update_platform_votes)
        (account_auth_platform)
        (account_cancel_auth_platform)
        (set_voting_proxy)
        (get_account)
        (get_full_account)
        (get_block)
        (get_account_count)
        (get_relative_account_history)
        //(is_public_key_registered)
        (get_global_properties)
        (get_dynamic_global_properties)
        (get_object)
        (get_private_key)
        (normalize_brain_key)
        //(set_wallet_filename)
        //(load_wallet_file)
        (save_wallet_file)
        (serialize_transaction)
        (sign_transaction)
        (get_prototype_operation)
        //(dbg_make_uia)
        //(dbg_push_blocks)
        //(dbg_generate_blocks)
        //(dbg_stream_json_objects)
        //(dbg_update_object)
        //(flood_network)
        (network_add_nodes)
        (network_get_connected_peers)
        //(set_key_label)
        //(get_key_label)
        (get_public_key)
        (score_a_post)
        (reward_post)
        (reward_post_proxy_by_platform)
        (buyout_post)
        (create_license)
        (create_post)
        (update_post)
        (account_manage)
        (get_post)
        (get_posts_by_platform_poster)
        (get_score)
        (list_scores)
        (get_license)
        (list_licenses)
        (get_advertising)
        (list_advertisings)
        (get_post_profits_detail)
        (get_platform_profits_detail)
        (get_poster_profits_detail)
        (get_score_profit)
        (get_account_statistics)
        (create_advertising)
        (update_advertising)
        (buy_advertising)
        (confirm_advertising)
        (ransom_advertising)
        (get_global_properties_extensions)
      )
