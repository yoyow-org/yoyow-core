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
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <numeric>

namespace graphene { namespace chain {
   class database;

   /**
    * @class account_statistics_object
    * @ingroup object
    * @ingroup implementation
    *
    * This object contains regularly updated statistical data about an account. It is provided for the purpose of
    * separating the account data that changes frequently from the account data that is mostly static, which will
    * minimize the amount of data that must be backed up as part of the undo history everytime a transfer is made.
    */
   class account_statistics_object : public graphene::db::abstract_object<account_statistics_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_account_statistics_object_type;

         account_uid_type  owner;

         /**
          * Keep the most recent operation as a root pointer to a linked list of the transaction history.
          */
         account_transaction_history_id_type most_recent_op;
         /** Total operations related to this account. */
         uint32_t                            total_ops = 0;
         /** Total operations related to this account that has been removed from the database. */
         uint32_t                            removed_ops = 0;

         /**
          * Prepaid fee.
          */
         share_type prepaid;
         /**
          * Coin-seconds-as-fee.
          */
         share_type csaf;

         /**
          * Core balance.
          */
         share_type core_balance;

         /**
          * As-fee coins that leased from others to this account.
          */
         share_type core_leased_in;
         /**
          * As-fee coins that leased from this account to others.
          */
         share_type core_leased_out;

         /**
          * Tracks average coins for calculating csaf of this account. Lazy updating.
          */
         share_type                     average_coins;
         /**
          * Tracks the most recent time when @ref average_coins was updated.
          */
         fc::time_point_sec             average_coins_last_update;

         /**
          * Tracks the coin-seconds earned by this account. Lazy updating.
          * actual_coin_seconds_earned = coin_seconds_earned + current_balance * (now - coin_seconds_earned_last_update)
          */
         // TODO maybe better to use a struct to store coin_seconds_earned and coin_seconds_earned_last_update
         //      and related functions e.g. compute_coin_seconds_earned and update_coin_seconds_earned
         fc::uint128_t                  coin_seconds_earned;

         /**
          * Tracks the most recent time when @ref coin_seconds_earned was updated.
          */
         fc::time_point_sec             coin_seconds_earned_last_update;

         /**
          * coins locked as witness pledge.
          */
         share_type total_witness_pledge;

         /**
          * coins that are requested to be released from witness pledge but not yet unlocked.
          */
         share_type releasing_witness_pledge;

         /**
          * block number that releasing witness pledge will be finally unlocked.
          */
         uint32_t witness_pledge_release_block_number = -1;

         /**
          * how many times have this account created witness object
          */
         uint32_t last_witness_sequence = 0;

         /**
          * uncollected witness pay.
          */
         share_type uncollected_witness_pay;

         /**
          * last produced block number
          */
         uint64_t witness_last_confirmed_block_num = 0;

         /**
          * last witness aslot
          */
         uint64_t witness_last_aslot = 0;

         /**
          * total blocks produced
          */
         uint64_t witness_total_produced = 0;

         /**
          * total blocks missed
          */
         uint64_t witness_total_missed = 0;

         /**
          * last reported block number
          */
         uint64_t witness_last_reported_block_num = 0;

         /**
          * total blocks reported
          */
         uint64_t witness_total_reported = 0;

         /**
          * coins locked as committee member pledge.
          */
         share_type total_committee_member_pledge;

         /**
          * coins that are requested to be released from committee member pledge but not yet unlocked.
          */
         share_type releasing_committee_member_pledge;

         /**
          * block number that releasing committee member pledge will be finally unlocked.
          */
         uint32_t committee_member_pledge_release_block_number = -1;

         /**
          * how many times have this account created committee member object
          */
         uint32_t last_committee_member_sequence = 0;

         /**
          * whether this account is permitted to be a governance voter.
          */
         bool can_vote = true;

         /**
          * whether this account is a governance voter.
          */
         bool is_voter = false;

         /**
          * how many times has this account became a voter
          */
         uint32_t last_voter_sequence = 0;

         /**
          * Record how many times the platform object has been created (the latest platform serial number)
          */
         uint32_t last_platform_sequence = 0;
         
         /**
          * Platform total deposit
          */
         share_type total_platform_pledge;

         /**
          * To refund the platform deposit
          */
         share_type releasing_platform_pledge;

         /**
          * block number that releasing platform pledge will be finally unlocked.
          */
         uint32_t platform_pledge_release_block_number = -1;

         /**
          * Record the last published article number
          */
         post_pid_type last_post_sequence = 0;

         /**
         * Record the last create custom vote number
         */
         custom_vote_vid_type last_custom_vote_sequence = 0; 
         advertising_aid_type last_advertising_sequence = 0;
         license_lid_type     last_license_sequence = 0;
         /**
          * Compute coin_seconds_earned.  Used to
          * non-destructively figure out how many coin seconds
          * are available.
          */
         // TODO use a public funtion to do this job as well as same job in vesting_balance_object
         std::pair<fc::uint128_t,share_type> compute_coin_seconds_earned(const uint64_t window, const fc::time_point_sec now, const bool reduce_witness = false)const;

         /**
          * Update coin_seconds_earned and
          * coin_seconds_earned_last_update fields due to passing of time
          */
         // TODO use a public funtion to do this job and same job in vesting_balance_object
         void update_coin_seconds_earned(const uint64_t window, const fc::time_point_sec now, const bool reduce_witness = false);

         /**
          * Update coin_seconds_earned and
          * coin_seconds_earned_last_update fields with new data
          */
         void set_coin_seconds_earned(const fc::uint128_t new_coin_seconds, const fc::time_point_sec now);
   };

   /**
    * @brief Tracks the balance of a single account/asset pair
    * @ingroup object
    *
    * This object is indexed on owner and asset_type so that black swan
    * events in asset_type can be processed quickly.
    */
   class account_balance_object : public abstract_object<account_balance_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_account_balance_object_type;

         account_uid_type  owner;
         asset_aid_type    asset_type;
         share_type        balance;

         asset get_balance()const { return asset(balance, asset_type); }
         void  adjust_balance(const asset& delta);
   };


   /**
    * @brief This class represents an account on the object graph
    * @ingroup object
    * @ingroup protocol
    *
    * Accounts are the primary unit of authority on the graphene system. Users must have an account in order to use
    * assets, trade in the markets, vote for committee_members, etc.
    */
   class account_object : public graphene::db::abstract_object<account_object>
   {
      public:
         static const uint8_t space_id = protocol_ids;
         static const uint8_t type_id  = account_object_type;

         /**
          * The time at which this account's membership expires.
          * If set to any time in the past, the account is a basic account.
          * If set to time_point_sec::maximum(), the account is a lifetime member.
          * If set to any time not in the past less than time_point_sec::maximum(), the account is an annual member.
          *
          * See @ref is_lifetime_member, @ref is_basic_account, @ref is_annual_member, and @ref is_member
          */
         time_point_sec membership_expiration_date;

         uint32_t         referrer_by_platform = 0;  //if referrered by platform, == platform.sequence
         ///The account that paid the fee to register this account. Receives a percentage of referral rewards.
         account_uid_type registrar;
         /// The account credited as referring this account. Receives a percentage of referral rewards.
         account_uid_type referrer;
         /// The lifetime member at the top of the referral tree. Receives a percentage of referral rewards.
         account_uid_type lifetime_referrer;

         /// Percentage of fee which should go to network.
         uint16_t network_fee_percentage = GRAPHENE_DEFAULT_NETWORK_PERCENT_OF_FEE;
         /// Percentage of fee which should go to lifetime referrer.
         uint16_t lifetime_referrer_fee_percentage = 0;
         /// Percentage of referral rewards (leftover fee after paying network and lifetime referrer) which should go
         /// to referrer. The remainder of referral rewards goes to the registrar.
         uint16_t referrer_rewards_percentage = 0;

         /// The account's uid. This uid must be unique among all account uids.
         account_uid_type uid = 0;

         /// The account's name. This name must be unique among all account names on the graph. May not be empty.
         string name;

         /**
          * The owner authority represents absolute control over the account. Usually the keys in this authority will
          * be kept in cold storage, as they should not be needed very often and compromise of these keys constitutes
          * complete and irrevocable loss of the account. Generally the only time the owner authority is required is to
          * update the active authority.
          */
         authority owner;
         /// The owner authority contains the hot keys of the account. This authority has control over nearly all
         /// operations the account may perform.
         authority active;
         /// The secondary authority has control over a few operations.
         authority secondary;

         public_key_type memo_key;

         account_reg_info reg_info;

         bool can_post = true;      //Default to the user posting permissions
         bool can_reply = false;
         bool can_rate = false;

         bool is_full_member = false;     //Currently mainly used for referral tags, that is, = true to refer other people; currently there is only a platform
         bool is_registrar = false;
         bool is_admin = false;

         time_point_sec create_time;
         time_point_sec last_update_time;

         string active_data = "{}";
         string secondary_data = "{}";

         /// The reference implementation records the account's statistics in a separate object. This field contains the
         /// ID of that object.
         account_statistics_id_type statistics;

         /**
          * This is a set of all accounts which have 'whitelisted' this account. Whitelisting is only used in core
          * validation for the purpose of authorizing accounts to hold and transact in whitelisted assets. This
          * account cannot update this set, except by transferring ownership of the account, which will clear it. Other
          * accounts may add or remove their IDs from this set.
          */
         flat_set<account_uid_type> whitelisting_accounts;

         /**
          * Optionally track all of the accounts this account has whitelisted or blacklisted, these should
          * be made Immutable so that when the account object is cloned no deep copy is required.  This state is
          * tracked for GUI display purposes.
          *
          * TODO: move white list tracking to its own multi-index container rather than having 4 fields on an
          * account.   This will scale better because under the current design if you whitelist 2000 accounts,
          * then every time someone fetches this account object they will get the full list of 2000 accounts.
          */
         ///@{
         set<account_uid_type> whitelisted_accounts;
         set<account_uid_type> blacklisted_accounts;
         ///@}


         /**
          * This is a set of all accounts which have 'blacklisted' this account. Blacklisting is only used in core
          * validation for the purpose of forbidding accounts from holding and transacting in whitelisted assets. This
          * account cannot update this set, and it will be preserved even if the account is transferred. Other accounts
          * may add or remove their IDs from this set.
          */
         flat_set<account_uid_type> blacklisting_accounts;

         /**
          * This is a set of assets which the account is allowed to have.
          * This is utilized to restrict buyback accounts to the assets that trade in their markets.
          * In the future we may expand this to allow accounts to e.g. voluntarily restrict incoming transfers.
          */
         optional< flat_set<asset_aid_type> > allowed_assets;

         /// @return true if this is a lifetime member account; false otherwise.
         bool is_lifetime_member()const
         {
            return membership_expiration_date == time_point_sec::maximum();
         }
         /// @return true if this is a basic account; false otherwise.
         bool is_basic_account(time_point_sec now)const
         {
            return now > membership_expiration_date;
         }
         /// @return true if the account is an unexpired annual member; false otherwise.
         /// @note This method will return false for lifetime members.
         bool is_annual_member(time_point_sec now)const
         {
            return !is_lifetime_member() && !is_basic_account(now);
         }
         /// @return true if the account is an annual or lifetime member; false otherwise.
         bool is_member(time_point_sec now)const
         {
            return !is_basic_account(now);
         }

         /// @return true if the account has enabled allowed_assets; false otherwise.
         bool enabled_allowed_assets()const { return allowed_assets.valid(); }

         account_id_type get_id()const { return id; }
         account_uid_type get_uid()const { return uid; }
   };

   /**
    * @brief This class represents a voting account on the object graph
    * @ingroup object
    * @ingroup protocol
    */
   class voter_object : public graphene::db::abstract_object<voter_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_voter_object_type;

         /// The account's uid. This uid must be unique among all account uids.
         account_uid_type    uid = 0;
         uint32_t            sequence = 0;
         bool                is_valid = true;

         uint64_t            votes = 0;
         fc::time_point_sec  votes_last_update;

         uint64_t            effective_votes = 0;
         fc::time_point_sec  effective_votes_last_update;
         uint32_t            effective_votes_next_update_block;

         account_uid_type    proxy_uid = GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID;
         uint32_t            proxy_sequence = 0;

         uint64_t            proxied_voters = 0;
         vector<uint64_t>    proxied_votes;         // [ level1, level2, ... ]
         vector<uint32_t>    proxy_last_vote_block; // [ self, proxy, proxy->proxy, ... ]

         uint32_t            effective_last_vote_block; // effective value, due to proxied voting

         uint16_t            number_of_witnesses_voted = 0; // directly voted
         uint16_t            number_of_platform_voted = 0;         // Direct investment platform number
         uint16_t            number_of_committee_members_voted = 0; // directly voted

         uint64_t total_votes() const
         {
            return std::accumulate( proxied_votes.begin(), proxied_votes.end(), effective_votes );
         }
         void update_effective_last_vote_block()
         {
            effective_last_vote_block = *std::max_element( proxy_last_vote_block.begin(), proxy_last_vote_block.end() );
         }
   };

   /**
    * @brief This class represents an account registrar takeover relationship on the object graph
    * @ingroup object
    * @ingroup protocol
    */
   class registrar_takeover_object : public graphene::db::abstract_object<registrar_takeover_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_registrar_takeover_object_type;

         /// The account's uid. This uid must be unique among all account uids.
         account_uid_type    original_registrar = 0;
         account_uid_type    takeover_registrar = 0;
   };

   /**
    *  @brief This secondary index will allow a reverse lookup of all accounts that a particular key or account
    *  is an potential signing authority.
    */
   class account_member_index : public secondary_index
   {
      public:
         virtual void object_inserted( const object& obj ) override;
         virtual void object_removed( const object& obj ) override;
         virtual void about_to_modify( const object& before ) override;
         virtual void object_modified( const object& after  ) override;


         /** given an account or key, map it to the set of accounts that reference it in an active or owner authority */
         map< account_uid_type, set<account_uid_type> > account_to_account_memberships;
         map< public_key_type, set<account_uid_type> > account_to_key_memberships;


      protected:
         set<account_uid_type>  get_account_members( const account_object& a )const;
         set<public_key_type>  get_key_members( const account_object& a )const;

         set<account_uid_type>  before_account_members;
         set<public_key_type>  before_key_members;
   };


   /**
    *  @brief This secondary index will allow a reverse lookup of all accounts that have been referred by
    *  a particular account.
    */
   class account_referrer_index : public secondary_index
   {
      public:
         virtual void object_inserted( const object& obj ) override;
         virtual void object_removed( const object& obj ) override;
         virtual void about_to_modify( const object& before ) override;
         virtual void object_modified( const object& after  ) override;

         /** maps the referrer to the set of accounts that they have referred */
         map< account_uid_type, set<account_uid_type> > referred_by;
   };

   struct by_account_asset;
   struct by_asset_balance;
   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      account_balance_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_account_asset>,
            composite_key<
               account_balance_object,
               member<account_balance_object, account_uid_type, &account_balance_object::owner>,
               member<account_balance_object, asset_aid_type, &account_balance_object::asset_type>
            >
         >,
         ordered_unique< tag<by_asset_balance>, // used by asset_api
            composite_key<
               account_balance_object,
               member<account_balance_object, asset_aid_type, &account_balance_object::asset_type>,
               member<account_balance_object, share_type, &account_balance_object::balance>,
               member<account_balance_object, account_uid_type, &account_balance_object::owner>
            >,
            composite_key_compare<
               std::less< asset_aid_type >,
               std::greater< share_type >,
               std::less< account_uid_type >
            >
         >
      >
   > account_balance_object_multi_index_type;

   /**
    * @ingroup object_index
    */
   typedef generic_index<account_balance_object, account_balance_object_multi_index_type> account_balance_index;

   struct by_name;
   struct by_uid;

   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      account_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_name>, member<account_object, string, &account_object::name> >,
         ordered_unique< tag<by_uid>, member<account_object, account_uid_type, &account_object::uid> >
      >
   > account_multi_index_type;

   /**
    * @ingroup object_index
    */
   typedef generic_index<account_object, account_multi_index_type> account_index;


   struct by_uid_seq;
   struct by_votes_next_update;
   struct by_last_vote;
   struct by_valid;
   struct by_proxy;

   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      voter_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_uid_seq>,
            composite_key<
               voter_object,
               member< voter_object, account_uid_type, &voter_object::uid>,
               member< voter_object, uint32_t, &voter_object::sequence>
            >
         >,
         ordered_unique< tag<by_votes_next_update>,
            composite_key<
               voter_object,
               member< voter_object, uint32_t, &voter_object::effective_votes_next_update_block>,
               member< voter_object, account_uid_type, &voter_object::uid>,
               member< voter_object, uint32_t, &voter_object::sequence>
            >
         >,
         ordered_unique< tag<by_last_vote>,
            composite_key<
               voter_object,
               member< voter_object, uint32_t, &voter_object::effective_last_vote_block>,
               member< voter_object, account_uid_type, &voter_object::uid>,
               member< voter_object, uint32_t, &voter_object::sequence>
            >
         >,
         ordered_unique< tag<by_valid>,
            composite_key<
               voter_object,
               member< voter_object, bool, &voter_object::is_valid>,
               member< voter_object, account_uid_type, &voter_object::proxy_uid>,
               member< voter_object, uint32_t, &voter_object::proxy_sequence>,
               member< voter_object, uint32_t, &voter_object::effective_last_vote_block>,
               member< voter_object, account_uid_type, &voter_object::uid>,
               member< voter_object, uint32_t, &voter_object::sequence>
            >
         >,
         ordered_unique< tag<by_proxy>,
            composite_key<
               voter_object,
               member< voter_object, account_uid_type, &voter_object::proxy_uid>,
               member< voter_object, uint32_t, &voter_object::proxy_sequence>,
               member< voter_object, account_uid_type, &voter_object::uid>,
               member< voter_object, uint32_t, &voter_object::sequence>
            >
         >
      >
   > voter_multi_index_type;

   /**
    * @ingroup object_index
    */
   typedef generic_index<voter_object, voter_multi_index_type> voter_index;


   struct by_original;
   struct by_takeover;

   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      registrar_takeover_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_original>,
            member< registrar_takeover_object, account_uid_type, &registrar_takeover_object::original_registrar>
         >,
         ordered_unique< tag<by_takeover>,
            composite_key<
               registrar_takeover_object,
               member< registrar_takeover_object, account_uid_type, &registrar_takeover_object::takeover_registrar>,
               member< registrar_takeover_object, account_uid_type, &registrar_takeover_object::original_registrar>
            >
         >
      >
   > registrar_takeover_multi_index_type;

   /**
    * @ingroup object_index
    */
   typedef generic_index<registrar_takeover_object, registrar_takeover_multi_index_type> registrar_takeover_index;


   struct by_witness_pledge_release;
   struct by_committee_member_pledge_release;
   struct by_platform_pledge_release;

   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      account_statistics_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_uid>, member<account_statistics_object, account_uid_type, &account_statistics_object::owner> >,
         ordered_unique< tag<by_witness_pledge_release>,
            composite_key<
               account_statistics_object,
               member<account_statistics_object, uint32_t, &account_statistics_object::witness_pledge_release_block_number>,
               member<account_statistics_object, account_uid_type, &account_statistics_object::owner>
            >
         >,
         ordered_unique< tag<by_committee_member_pledge_release>,
            composite_key<
               account_statistics_object,
               member<account_statistics_object, uint32_t, &account_statistics_object::committee_member_pledge_release_block_number>,
               member<account_statistics_object, account_uid_type, &account_statistics_object::owner>
            >
         >,
         ordered_unique< tag<by_platform_pledge_release>,
            composite_key<
               account_statistics_object,
               member<account_statistics_object, uint32_t, &account_statistics_object::platform_pledge_release_block_number>,
               member<account_statistics_object, account_uid_type, &account_statistics_object::owner>
            >
         >
      >
   > account_statistics_object_multi_index_type;

   /**
    * @ingroup object_index
    */
   typedef generic_index<account_statistics_object, account_statistics_object_multi_index_type> account_statistics_index;


   class account_auth_platform_object : public graphene::db::abstract_object<account_auth_platform_object>
   {
   public:
      enum Platform_Auth_Permission
      {
         Platform_Permission_Forward        = 1,        //allow forward 
         Platform_Permission_Liked          = 2,        //allow liked or scored
         Platform_Permission_Buyout         = 4,        //allow buyout
         Platform_Permission_Comment        = 8,        //allow comment
         Platform_Permission_Reward         = 16,       //allow reward
         Platform_Permission_Transfer       = 32,       //allow transfer
         Platform_Permission_Post           = 64,       //allow post
         Platform_Permission_Content_Update = 128 //allow content update
      };

      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id = impl_account_auth_platform_object_type;

      account_uid_type  account;
      account_uid_type  platform;

      share_type    max_limit = 0;   //max limit prepaid for platform
      share_type    cur_used = 0;    //current prepaid used by platform 
      bool          is_active = true;
      uint32_t      permission_flags = account_auth_platform_object::Platform_Permission_Forward |
                                       account_auth_platform_object::Platform_Permission_Liked |
                                       account_auth_platform_object::Platform_Permission_Buyout |
                                       account_auth_platform_object::Platform_Permission_Comment |
                                       account_auth_platform_object::Platform_Permission_Reward |
                                       account_auth_platform_object::Platform_Permission_Transfer |
                                       account_auth_platform_object::Platform_Permission_Post |
                                       account_auth_platform_object::Platform_Permission_Content_Update;
      optional<memo_data>    memo;

      share_type get_auth_platform_usable_prepaid(share_type account_prepaid) const{
         share_type usable_prepaid = 0;
         FC_ASSERT(max_limit >= cur_used);
         share_type amount = max_limit - cur_used;
         usable_prepaid = account_prepaid >= amount ? amount : account_prepaid;
         return usable_prepaid;
      };
   };

   struct by_account_uid{};
   struct by_platform_uid{};
   struct by_account_platform{};
   struct by_platform_account{};

   typedef multi_index_container<
       account_auth_platform_object,
	    indexed_by<
	      ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_account_platform>,
                                 composite_key< 
                                 account_auth_platform_object,
                                 member< account_auth_platform_object, account_uid_type, &account_auth_platform_object::account >,
                                 member< account_auth_platform_object, account_uid_type, &account_auth_platform_object::platform >>
                                 >,
         ordered_unique< tag<by_platform_account>,
                                 composite_key< 
                                 account_auth_platform_object,
                                 member< account_auth_platform_object, account_uid_type, &account_auth_platform_object::platform >,
                                 member< account_auth_platform_object, account_uid_type, &account_auth_platform_object::account >>>
       >
   > account_auth_platform_multi_index_type;

   /**
   * @ingroup object_index
   */
   typedef generic_index<account_auth_platform_object, account_auth_platform_multi_index_type> account_auth_platform_index;

}}

FC_REFLECT_DERIVED( graphene::chain::account_object,
                    (graphene::db::object),
                    //(membership_expiration_date)
                    (referrer_by_platform)
                    //(registrar)(referrer)(lifetime_referrer)
                    //(network_fee_percentage)(lifetime_referrer_fee_percentage)(referrer_rewards_percentage)
                    (uid)(name)(owner)(active)(secondary)(memo_key)(reg_info)
                    (can_post)(can_reply)(can_rate)
                    (is_full_member)(is_registrar)(is_admin)
                    (create_time)(last_update_time)
                    (active_data)(secondary_data)
                    (statistics)
                    //(whitelisting_accounts)(blacklisting_accounts)
                    //(whitelisted_accounts)(blacklisted_accounts)
                    (allowed_assets)
                  )

FC_REFLECT_DERIVED( graphene::chain::voter_object,
                    (graphene::db::object),
                    (uid)(sequence)
                    (is_valid)
                    (votes)(votes_last_update)
                    (effective_votes)(effective_votes_last_update)(effective_votes_next_update_block)
                    (proxy_uid)(proxy_sequence)
                    (proxied_voters)(proxied_votes)(proxy_last_vote_block)
                    (effective_last_vote_block)
                    (number_of_witnesses_voted)
                    (number_of_platform_voted)
                    (number_of_committee_members_voted)
                  )

FC_REFLECT_DERIVED( graphene::chain::registrar_takeover_object,
                    (graphene::db::object),
                    (original_registrar)(takeover_registrar) )

FC_REFLECT_DERIVED( graphene::chain::account_balance_object,
                    (graphene::db::object),
                    (owner)(asset_type)(balance) )

FC_REFLECT_DERIVED( graphene::chain::account_statistics_object,
                    (graphene::chain::object),
                    (owner)
                    //(most_recent_op)
                    (total_ops)
                    (removed_ops)
                    (prepaid)(csaf)
                    (core_balance)(core_leased_in)(core_leased_out)
                    (average_coins)(average_coins_last_update)
                    (coin_seconds_earned)(coin_seconds_earned_last_update)
                    (total_witness_pledge)(releasing_witness_pledge)(witness_pledge_release_block_number)
                    (last_witness_sequence)(uncollected_witness_pay)
                    (witness_last_confirmed_block_num)
                    (witness_last_aslot)
                    (witness_total_produced)
                    (witness_total_missed)
                    (witness_last_reported_block_num)
                    (witness_total_reported)
                    (total_committee_member_pledge)(releasing_committee_member_pledge)(committee_member_pledge_release_block_number)
                    (last_committee_member_sequence)
                    (can_vote)(is_voter)(last_voter_sequence)
                    (last_platform_sequence)
                    (total_platform_pledge)
                    (releasing_platform_pledge)
                    (platform_pledge_release_block_number)
                    (last_post_sequence)
                    (last_custom_vote_sequence)
                    (last_advertising_sequence)
                    (last_license_sequence)
                  )

FC_REFLECT_DERIVED(graphene::chain::account_auth_platform_object,
                  (graphene::db::object),
                  (account)(platform)
                  (max_limit)(cur_used)(is_active)(permission_flags)(memo))
