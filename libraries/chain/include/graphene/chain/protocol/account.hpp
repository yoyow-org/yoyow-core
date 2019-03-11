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
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/ext.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/memo.hpp>

namespace graphene { namespace chain {

   /// Account registration related info.
   struct account_reg_info
   {
      /// This account pays the fee.
      account_uid_type registrar = GRAPHENE_NULL_ACCOUNT_UID;

      /// This account receives a portion of the reward split between registrar and referrer.
      account_uid_type referrer = GRAPHENE_NULL_ACCOUNT_UID;

      /// The percentages go to the registrar and the referrer.
      uint16_t        registrar_percent = 0;
      uint16_t        referrer_percent = 0;
      asset           allowance_per_article;
      asset           max_share_per_article;
      asset           max_share_total;
      uint16_t        buyout_percent = GRAPHENE_100_PERCENT;

      extensions_type extensions;

      void validate()const;
   };

   /**
    *  @ingroup operations
    *  @brief Create an account
    */
   struct account_create_operation : public base_operation
   {
      struct fee_parameters_type
      {
         uint64_t basic_fee        = 5*GRAPHENE_BLOCKCHAIN_PRECISION; ///< the cost to register the cheapest non-free account
         //uint64_t premium_fee      = 2000*GRAPHENE_BLOCKCHAIN_PRECISION; ///< the cost to register the cheapest non-free account
         uint32_t price_per_auth   = GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee     = 0;
         uint16_t min_rf_percent   = 0;
         extensions_type        extensions;
      };

      fee_type        fee;

      account_uid_type uid;

      string          name;
      authority       owner;
      authority       active;
      authority       secondary;

      public_key_type memo_key;

      account_reg_info reg_info;

      extensions_type extensions;

      account_uid_type fee_payer_uid()const { return reg_info.registrar; }
      void             validate()const;
      share_type       calculate_fee(const fee_parameters_type& )const;

      void get_required_active_uid_authorities( flat_set<account_uid_type>& a )const
      {
         // registrar should be required anyway as it is the fee_payer_uid(),
         // but we insert it here because fee can be paid with secondary authority
         a.insert( reg_info.registrar );
      }
   };

   /**
    *  @ingroup operations
    *  @brief Manage an existing account
    */
   struct account_manage_operation : public base_operation
   {

      struct opt
      {
         optional< bool > can_post;
         optional< bool > can_reply;
         optional< bool > can_rate;
      };

      struct fee_parameters_type
      {
         uint64_t fee              = 1*GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee     = 0;
         uint16_t min_rf_percent   = 0;
         extensions_type     extensions;
      };

      fee_type        fee;

      account_uid_type executor;
      account_uid_type account;
      extension< opt > options;

      extensions_type extensions;

      account_uid_type fee_payer_uid()const { return executor; }
      void             validate()const;
      //use default
      //share_type      calculate_fee(const fee_parameters_type& )const;

      void get_required_active_uid_authorities( flat_set<account_uid_type>& a )const
      {
         // executor should be required anyway as it is the fee_payer_uid(),
         // but we insert it here because fee can be paid with secondary authority
         a.insert( executor );
      }
   };

   /**
    *  @ingroup operations
    *  @brief Replace a key in an account with a new key. Need to be signed with the old key.
    */
   struct account_update_key_operation : public base_operation
   {

      struct fee_parameters_type
      {
         uint64_t fee              = 1*GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee     = 0;
         uint16_t min_rf_percent   = 0;
         extensions_type        extensions;
      };

      fee_type         fee;

      account_uid_type fee_paying_account;

      account_uid_type uid;

      public_key_type  old_key;
      public_key_type  new_key;
      // for security reason, don't update owner with this op
      bool             update_active;
      bool             update_secondary;
      // don't update memo key with this op as well

      extensions_type extensions;

      account_uid_type fee_payer_uid()const { return fee_paying_account; }
      void             validate()const;
      //use default
      //share_type       calculate_fee(const fee_parameters_type& )const;

      void get_required_authorities( vector<authority>& v )const
      {
         v.push_back( authority( 1, old_key, 1) );
      }
   };

   /**
    *  @ingroup operations
    *  @brief Update account authorities and/or memo key
    */
   struct account_update_auth_operation : public base_operation
   {

      struct fee_parameters_type
      {
         uint64_t fee              = 1*GRAPHENE_BLOCKCHAIN_PRECISION;
         uint32_t price_per_auth   = GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee     = 0;
         uint16_t min_rf_percent   = 0;
         extensions_type        extensions;
      };

      fee_type         fee;

      account_uid_type uid;

      optional<authority> owner;
      optional<authority> active;
      optional<authority> secondary;
      optional<public_key_type> memo_key;

      extensions_type extensions;

      account_uid_type fee_payer_uid()const { return uid; }
      void             validate()const;
      share_type       calculate_fee(const fee_parameters_type& )const;

      void get_required_owner_uid_authorities( flat_set<account_uid_type>& a )const
      {
         // update of owner or active authority requires owner authority
         if( owner.valid() || active.valid() )
            a.insert( uid );
      }
      void get_required_active_uid_authorities( flat_set<account_uid_type>& a )const
      {
         // update of other data requires active authority
         if( !( owner.valid() || active.valid() ) )
            a.insert( uid );
      }
   };

   /**
    * @brief account grant of authorities to platform 
    * @ingroup operations
    */
   struct account_auth_platform_operation : public base_operation
   {
	   struct extension_parameter
	   {
		   optional<share_type> limit_for_platform;
           optional<uint32_t>   permission_flags;
           optional<memo_data>  memo;
	   };

      struct fee_parameters_type
      {
         uint64_t fee              = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee     = 0;
         uint16_t min_rf_percent   = 0;
         extensions_type   extensions;
      };

      fee_type                   fee;
      account_uid_type           uid;
      account_uid_type           platform;

      optional< extension<extension_parameter> > extensions;

      account_uid_type  fee_payer_uid()const { return uid; }
      void              validate()const;
      //share_type      calculate_fee(const fee_parameters_type& k)const;
      void get_required_active_uid_authorities( flat_set<account_uid_type>& a )const
      {
         // need active authority
         a.insert( uid );
      }
   };

   /**
    * @brief account ungrant of authorities to platform 
    * @ingroup operations
    */
   struct account_cancel_auth_platform_operation : public base_operation
   {
      struct fee_parameters_type
      {
         uint64_t fee              = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee     = 0;
         uint16_t min_rf_percent   = 0;
         extensions_type   extensions;
      };

      fee_type                   fee;
      account_uid_type           uid;
      account_uid_type           platform;

      extensions_type   extensions;

      account_uid_type  fee_payer_uid()const { return uid; }
      void              validate()const;
      //share_type      calculate_fee(const fee_parameters_type& k)const;
      void get_required_active_uid_authorities( flat_set<account_uid_type>& a )const
      {
         // need active authority
         a.insert( uid );
      }
   };

  /**
    * @brief Change witness voting proxy.
    * @ingroup operations
    */
   struct account_update_proxy_operation : public base_operation
   {
      struct fee_parameters_type
      {
         uint64_t fee              = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee     = 0;
         uint16_t min_rf_percent   = 0;
         extensions_type   extensions;
      };

      fee_type                   fee;
      /// The account which vote for witnesses. This account pays the fee for this operation.
      account_uid_type           voter;
      account_uid_type           proxy;

      extensions_type   extensions;

      account_uid_type  fee_payer_uid()const { return voter; }
      void              validate()const;
      //share_type      calculate_fee(const fee_parameters_type& k)const;
      void get_required_active_uid_authorities( flat_set<account_uid_type>& a )const
      {
         // need active authority
         a.insert( voter );
      }
   };

   /**
    * @brief account enable or disable allowed_assets attribute
    * @ingroup operations
    */
   struct account_enable_allowed_assets_operation : public base_operation
   {
      struct fee_parameters_type
      {
         uint64_t fee              = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee     = 0;
         uint16_t min_rf_percent   = 0;
         extensions_type   extensions;
      };

      fee_type          fee;
      account_uid_type  account;
      bool              enable = true;

      extensions_type   extensions;

      account_uid_type  fee_payer_uid()const { return account; }
      void              validate()const;
      //share_type        calculate_fee(const fee_parameters_type& k)const; // use default

      void get_required_active_uid_authorities( flat_set<account_uid_type>& a )const
      {
         // need active authority
         a.insert( account );
      }
   };

   /**
    * @brief account update allowed_assets attribute
    * @ingroup operations
    */
   struct account_update_allowed_assets_operation : public base_operation
   {
      struct fee_parameters_type
      {
         uint64_t fee              = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t price_per_asset  = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t min_real_fee     = 0;
         uint16_t min_rf_percent   = 0;
         extensions_type   extensions;
      };

      fee_type                   fee;
      account_uid_type           account;
      flat_set<asset_aid_type>   assets_to_add;
      flat_set<asset_aid_type>   assets_to_remove;

      extensions_type   extensions;

      account_uid_type  fee_payer_uid()const { return account; }
      void              validate()const;
      share_type        calculate_fee(const fee_parameters_type& k)const;

      void get_required_active_uid_authorities( flat_set<account_uid_type>& a )const
      {
         // need active authority if to remove any asset
         if( !assets_to_remove.empty() )
            a.insert( account );
      }
      void get_required_secondary_uid_authorities( flat_set<account_uid_type>& a )const
      {
         // need secondary authority if only to add
         if( assets_to_remove.empty() )
            a.insert( account );
      }
   };

   /**
    * @brief This operation is used to whitelist and blacklist accounts, primarily for transacting in whitelisted assets
    * @ingroup operations
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
    * This operation requires authorizing_account's signature, but not account_to_list's. The fee is paid by
    * authorizing_account.
    */
   struct account_whitelist_operation : public base_operation
   {
      struct fee_parameters_type { share_type fee = 300000; };
      enum account_listing {
         no_listing = 0x0, ///< No opinion is specified about this account
         white_listed = 0x1, ///< This account is whitelisted, but not blacklisted
         black_listed = 0x2, ///< This account is blacklisted, but not whitelisted
         white_and_black_listed = white_listed | black_listed ///< This account is both whitelisted and blacklisted
      };

      /// Paid by authorizing_account
      asset           fee;
      /// The account which is specifying an opinion of another account
      account_uid_type authorizing_account;
      /// The account being opined about
      account_uid_type account_to_list;
      /// The new white and blacklist status of account_to_list, as determined by authorizing_account
      /// This is a bitfield using values defined in the account_listing enum
      uint8_t new_listing = no_listing;
      extensions_type extensions;

      account_uid_type fee_payer_uid()const { return authorizing_account; }
      void validate()const { FC_ASSERT( fee.amount >= 0 ); FC_ASSERT(new_listing < 0x4); }
   };

} } // graphene::chain

FC_REFLECT(graphene::chain::account_reg_info, (registrar)(referrer)(registrar_percent)(referrer_percent)
                                              (allowance_per_article)(max_share_per_article)(max_share_total)(buyout_percent)
                                              (extensions))

FC_REFLECT_ENUM( graphene::chain::account_whitelist_operation::account_listing,
                (no_listing)(white_listed)(black_listed)(white_and_black_listed))

FC_REFLECT( graphene::chain::account_create_operation,
            (fee)
            (uid)
            (name)(owner)(active)(secondary)
            (memo_key)
            (reg_info)
            (extensions)
          )

FC_REFLECT(graphene::chain::account_manage_operation::opt, (can_post)(can_reply)(can_rate) )
FC_REFLECT( graphene::chain::account_manage_operation,
            (fee)
            (executor)
            (account)
            (options)
            (extensions)
          )

FC_REFLECT( graphene::chain::account_update_key_operation,
            (fee)(fee_paying_account)(uid)(old_key)(new_key)(update_active)(update_secondary)(extensions)
          )

FC_REFLECT( graphene::chain::account_update_auth_operation,
            (fee)(uid)(owner)(active)(secondary)(memo_key)(extensions)
          )

FC_REFLECT( graphene::chain::account_update_proxy_operation, (fee)(voter)(proxy)(extensions) )

FC_REFLECT(graphene::chain::account_auth_platform_operation::extension_parameter, (limit_for_platform)(permission_flags)(memo))
FC_REFLECT( graphene::chain::account_auth_platform_operation, (fee)(uid)(platform)(extensions) )
FC_REFLECT( graphene::chain::account_auth_platform_operation::fee_parameters_type,(fee)(min_real_fee)(min_rf_percent)(extensions) )

FC_REFLECT( graphene::chain::account_cancel_auth_platform_operation, (fee)(uid)(platform)(extensions) )
FC_REFLECT( graphene::chain::account_cancel_auth_platform_operation::fee_parameters_type,(fee)(min_real_fee)(min_rf_percent)(extensions) )

FC_REFLECT( graphene::chain::account_create_operation::fee_parameters_type,
            (basic_fee)
            (price_per_auth)(min_real_fee)(min_rf_percent)(extensions) )
FC_REFLECT( graphene::chain::account_manage_operation::fee_parameters_type, (fee)(min_real_fee)(min_rf_percent)(extensions) )
FC_REFLECT( graphene::chain::account_update_key_operation::fee_parameters_type, (fee)(min_real_fee)(min_rf_percent)(extensions) )
FC_REFLECT( graphene::chain::account_update_auth_operation::fee_parameters_type,
            (fee)(price_per_auth)(min_real_fee)(min_rf_percent)(extensions) )
FC_REFLECT( graphene::chain::account_update_proxy_operation::fee_parameters_type, (fee)(min_real_fee)(min_rf_percent)(extensions) )

FC_REFLECT( graphene::chain::account_enable_allowed_assets_operation,
            (fee)(account)(enable)(extensions) )
FC_REFLECT( graphene::chain::account_enable_allowed_assets_operation::fee_parameters_type,
            (fee)(min_real_fee)(min_rf_percent)(extensions) )

FC_REFLECT( graphene::chain::account_update_allowed_assets_operation,
            (fee)(account)(assets_to_add)(assets_to_remove)(extensions) )
FC_REFLECT( graphene::chain::account_update_allowed_assets_operation::fee_parameters_type,
            (fee)(price_per_asset)(min_real_fee)(min_rf_percent)(extensions) )

FC_REFLECT( graphene::chain::account_whitelist_operation, (fee)(authorizing_account)(account_to_list)(new_listing)(extensions))
FC_REFLECT( graphene::chain::account_whitelist_operation::fee_parameters_type, (fee) )
