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
#include <graphene/chain/protocol/memo.hpp>

namespace graphene { namespace chain { 

   bool is_valid_symbol( const string& symbol );

   /**
    * @brief The asset_options struct contains options available on all assets in the network
    *
    * @note Changes to this struct will break protocol compatibility
    */
   struct asset_options {
      /// The maximum supply of this asset which may exist at any given time. This can be as large as
      /// GRAPHENE_MAX_SHARE_SUPPLY
      share_type max_supply = GRAPHENE_MAX_SHARE_SUPPLY;
      /// When this asset is traded on the markets, this percentage of the total traded will be exacted and paid
      /// to the issuer. This is a fixed point value, representing hundredths of a percent, i.e. a value of 100
      /// in this field means a 1% fee is charged on market trades of this asset.
      uint16_t market_fee_percent = 0;
      /// Market fees calculated as @ref market_fee_percent of the traded volume are capped to this value
      share_type max_market_fee = GRAPHENE_MAX_SHARE_SUPPLY;

      /// The flags which the issuer has permission to update. See @ref asset_issuer_permission_flags
      uint16_t issuer_permissions = UIA_ASSET_ISSUER_PERMISSION_MASK;
      /// The currently active flags on this permission. See @ref asset_issuer_permission_flags
      uint16_t flags = 0;

      /// A set of accounts which maintain whitelists to consult for this asset. If whitelist_authorities
      /// is non-empty, then only accounts in whitelist_authorities are allowed to hold, use, or transfer the asset.
      flat_set<account_uid_type> whitelist_authorities;
      /// A set of accounts which maintain blacklists to consult for this asset. If flags & white_list is set,
      /// an account may only send, receive, trade, etc. in this asset if none of these accounts appears in
      /// its account_object::blacklisting_accounts field. If the account is blacklisted, it may not transact in
      /// this asset even if it is also whitelisted.
      flat_set<account_uid_type> blacklist_authorities;

      /** defines the assets that this asset may be traded against in the market */
      flat_set<asset_aid_type>   whitelist_markets;
      /** defines the assets that this asset may not be traded against in the market, must not overlap whitelist */
      flat_set<asset_aid_type>   blacklist_markets;

      /**
       * data that describes the meaning/purpose of this asset, fee will be charged proportional to
       * size of description.
       */
      string description;
      extensions_type extensions;

      /// Perform internal consistency checks.
      /// @throws fc::exception if any check fails
      void validate()const;
   };



   /**
    * @ingroup operations
    */
   struct asset_create_operation : public base_operation
   {
      struct fee_parameters_type { 
         uint64_t symbol3        = 500000 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t symbol4        = 300000 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t long_symbol    = 5000   * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint32_t price_per_kbyte = 10; /// only required for large memos.
      };

      fee_type                fee;
      /// This account must sign and pay the fee for this operation. Later, this account may update the asset
      account_uid_type        issuer;
      /// The ticker symbol of this asset
      string                  symbol;
      /// Number of digits to the right of decimal point, must be less than or equal to 12
      uint8_t                 precision = 0;

      /// Options common to all assets.
      ///
      /// @note common_options.core_exchange_rate technically needs to store the asset ID of this new asset. Since this
      /// ID is not known at the time this operation is created, create this price as though the new asset has instance
      /// ID 1, and the chain will overwrite it with the new asset's ID.
      asset_options              common_options;
      
      extensions_type extensions;

      account_uid_type fee_payer_uid()const { return issuer; }
      void            validate()const;
      share_type      calculate_fee( const fee_parameters_type& k )const;
   };

   /**
    * @brief Update options common to all assets
    * @ingroup operations
    *
    * There are a number of options which all assets in the network use. These options are enumerated in the @ref
    * asset_options struct. This operation is used to update these options for an existing asset.
    *
    * @note This operation cannot be used to update BitAsset-specific options. For these options, use @ref
    * asset_update_bitasset_operation instead.
    *
    * @pre @ref issuer SHALL be an existing account and MUST match asset_object::issuer on @ref asset_to_update
    * @pre @ref fee SHALL be nonnegative, and @ref issuer MUST have a sufficient balance to pay it
    * @pre @ref new_options SHALL be internally consistent, as verified by @ref validate()
    * @post @ref asset_to_update will have options matching those of new_options
    */
   struct asset_update_operation : public base_operation
   {
      struct fee_parameters_type { 
         uint64_t fee            = 500 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint32_t price_per_kbyte = 10;
      };

      asset_update_operation(){}

      fee_type         fee;
      account_uid_type issuer;
      asset_aid_type   asset_to_update;

      /// If the asset is to be given a new issuer, specify his ID here.
      optional<account_uid_type>   new_issuer;
      asset_options               new_options;
      extensions_type             extensions;

      account_uid_type fee_payer_uid()const { return issuer; }
      void            validate()const;
      share_type      calculate_fee(const fee_parameters_type& k)const;
   };

   /**
    * @ingroup operations
    */
   struct asset_issue_operation : public base_operation
   {
      struct fee_parameters_type { 
         uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION; 
         uint32_t price_per_kbyte = GRAPHENE_BLOCKCHAIN_PRECISION;
      };

      fee_type          fee;
      account_uid_type  issuer; ///< Must be asset_to_issue->asset_id->issuer
      asset             asset_to_issue;
      account_uid_type  issue_to_account;


      /** user provided data encrypted to the memo key of the "to" account */
      optional<memo_data>  memo;
      extensions_type      extensions;

      account_uid_type fee_payer_uid()const { return issuer; }
      void            validate()const;
      share_type      calculate_fee(const fee_parameters_type& k)const;
   };

   /**
    * @brief used to take an asset out of circulation, returning to the issuer
    * @ingroup operations
    *
    * @note You cannot use this operation on market-issued assets.
    */
   struct asset_reserve_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION; };

      fee_type          fee;
      account_uid_type  payer;
      asset             amount_to_reserve;
      extensions_type   extensions;

      account_uid_type fee_payer_uid()const { return payer; }
      void            validate()const;
   };

   /**
    * @brief used to transfer accumulated fees back to the issuer's balance.
    */
   struct asset_claim_fees_operation : public base_operation
   {
      struct fee_parameters_type {
         uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION;
      };

      fee_type         fee;
      account_uid_type issuer;
      asset            amount_to_claim; /// amount_to_claim.asset_id->issuer must == issuer
      extensions_type  extensions;

      account_uid_type fee_payer_uid()const { return issuer; }
      void            validate()const;
   };


} } // graphene::chain

FC_REFLECT( graphene::chain::asset_claim_fees_operation, (fee)(issuer)(amount_to_claim)(extensions) )
FC_REFLECT( graphene::chain::asset_claim_fees_operation::fee_parameters_type, (fee) )

FC_REFLECT( graphene::chain::asset_options,
            (max_supply)
            (market_fee_percent)
            (max_market_fee)
            (issuer_permissions)
            (flags)
            (whitelist_authorities)
            (blacklist_authorities)
            (whitelist_markets)
            (blacklist_markets)
            (description)
            (extensions)
          )

FC_REFLECT( graphene::chain::asset_create_operation::fee_parameters_type, (symbol3)(symbol4)(long_symbol)(price_per_kbyte) )
FC_REFLECT( graphene::chain::asset_update_operation::fee_parameters_type, (fee)(price_per_kbyte) )
FC_REFLECT( graphene::chain::asset_issue_operation::fee_parameters_type, (fee)(price_per_kbyte) )
FC_REFLECT( graphene::chain::asset_reserve_operation::fee_parameters_type, (fee) )


FC_REFLECT( graphene::chain::asset_create_operation,
            (fee)
            (issuer)
            (symbol)
            (precision)
            (common_options)
            (extensions)
          )
FC_REFLECT( graphene::chain::asset_update_operation,
            (fee)
            (issuer)
            (asset_to_update)
            (new_issuer)
            (new_options)
            (extensions)
          )

FC_REFLECT( graphene::chain::asset_issue_operation,
            (fee)(issuer)(asset_to_issue)(issue_to_account)(memo)(extensions) )
FC_REFLECT( graphene::chain::asset_reserve_operation,
            (fee)(payer)(amount_to_reserve)(extensions) )