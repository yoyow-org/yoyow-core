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

#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/asset.hpp>
#include <graphene/chain/protocol/authority.hpp>
#include <graphene/chain/protocol/ext.hpp>
#include <boost/tti/has_member_data.hpp>



namespace graphene { namespace chain {

   /**
    *  @defgroup operations Operations
    *  @ingroup transactions Transactions
    *  @brief A set of valid comands for mutating the globally shared state.
    *
    *  An operation can be thought of like a function that will modify the global
    *  shared state of the blockchain.  The members of each struct are like function
    *  arguments and each operation can potentially generate a return value.
    *
    *  Operations can be grouped into transactions (@ref transaction) to ensure that they occur
    *  in a particular order and that all operations apply successfully or
    *  no operations apply.
    *
    *  Each operation is a fully defined state transition and can exist in a transaction on its own.
    *
    *  @section operation_design_principles Design Principles
    *
    *  Operations have been carefully designed to include all of the information necessary to
    *  interpret them outside the context of the blockchain.   This means that information about
    *  current chain state is included in the operation even though it could be inferred from
    *  a subset of the data.   This makes the expected outcome of each operation well defined and
    *  easily understood without access to chain state.
    *
    *  @subsection balance_calculation Balance Calculation Principle
    *
    *    We have stipulated that the current account balance may be entirely calculated from
    *    just the subset of operations that are relevant to that account.  There should be
    *    no need to process the entire blockchain inorder to know your account's balance.
    *
    *  @subsection fee_calculation Explicit Fee Principle
    *
    *    Blockchain fees can change from time to time and it is important that a signed
    *    transaction explicitly agree to the fees it will be paying.  This aids with account
    *    balance updates and ensures that the sender agreed to the fee prior to making the
    *    transaction.
    *
    *  @subsection defined_authority Explicit Authority
    *
    *    Each operation shall contain enough information to know which accounts must authorize
    *    the operation.  This principle enables authority verification to occur in a centralized,
    *    optimized, and parallel manner.
    *
    *  @subsection relevancy_principle Explicit Relevant Accounts
    *
    *    Each operation contains enough information to enumerate all accounts for which the
    *    operation should apear in its account history.  This principle enables us to easily
    *    define and enforce the @balance_calculation. This is superset of the @ref defined_authority
    *
    *  @{
    */

   struct void_result{};
   typedef fc::static_variant<void_result,object_id_type,asset> operation_result;

   struct fee_extension_type
   {
      optional<asset> from_balance;
      optional<asset> from_prepaid;
      optional<asset> from_csaf;
   };
   typedef optional< extension< fee_extension_type > > fee_options_type;

   struct fee_type
   {
      fee_type( asset fee = asset() ) : total(fee) {}

      asset            total;
      fee_options_type options;
   };

   BOOST_TTI_HAS_MEMBER_DATA(min_real_fee)

   struct base_operation
   {
      template<typename T>
      share_type calculate_fee(const T& params)const
      {
         return params.fee;
      }
      template<typename T>
      std::pair<share_type, share_type> calculate_fee_pair(share_type fee, const T& params)const
      {
         FC_ASSERT( fee <= GRAPHENE_MAX_SHARE_SUPPLY );
         const bool b = has_member_data_min_real_fee<T,uint64_t>::value;
         return calculate_fee_pair( fee, params, std::integral_constant<bool, b>() );
      }
      template<typename T>
      std::pair<share_type, share_type> calculate_fee_pair(share_type fee, const T& params, std::true_type)const
      {
         fc::uint128 min_percent_fee = ( fc::uint128( fee.value ) * params.min_rf_percent ) / GRAPHENE_100_PERCENT;
         share_type min_real_fee = std::max( params.min_real_fee, min_percent_fee.to_uint64() );
         FC_ASSERT( min_real_fee <= GRAPHENE_MAX_SHARE_SUPPLY );

         return std::make_pair( fee, min_real_fee );
      }
      template<typename T>
      std::pair<share_type, share_type> calculate_fee_pair(share_type fee, const T& params, std::false_type)const
      {
         return std::make_pair( fee, fee );
      }
      account_id_type fee_payer()const { FC_ASSERT(false, "deprecated."); return account_id_type(); }
      account_uid_type fee_payer_uid()const { return GRAPHENE_TEMP_ACCOUNT_UID; }
      void get_required_authorities( vector<authority>& )const{}
      void get_required_active_authorities( flat_set<account_id_type>& )const{}
      void get_required_owner_authorities( flat_set<account_id_type>& )const{}
      void get_required_owner_uid_authorities( flat_set<account_uid_type>& )const{}
      void get_required_active_uid_authorities( flat_set<account_uid_type>& )const{}
      void get_required_secondary_uid_authorities( flat_set<account_uid_type>& )const{}
      void validate()const{}

      static uint64_t calculate_data_fee( uint64_t bytes, uint64_t price_per_kbyte );
   };

   /**
    *  For future expansion many structus include a single member of type
    *  extensions_type that can be changed when updating a protocol.  You can
    *  always add new types to a static_variant without breaking backward
    *  compatibility.   
    */
   typedef static_variant<void_t>      future_extensions;
   struct default_extension_type {};

   /**
    *  A flat_set is used to make sure that only one extension of
    *  each type is added and that they are added in order.  
    *  
    *  @note static_variant compares only the type tag and not the 
    *  content.
    */
   //typedef flat_set<future_extensions> extensions_type;
   typedef optional< extension< default_extension_type > > extensions_type;

   void validate_account_uid( const account_uid_type uid, const string& object_name = "" );
   void validate_asset_id( const asset& a, asset_aid_type aid, const string& object_name = "asset" );
   void validate_core_asset_id( const asset& a, const string& object_name = "asset" );
   void validate_non_core_asset_id( const asset& a, const string& object_name = "asset" );
   void validate_op_fee( const asset& fee, const string& op_name = "" );
   void validate_op_fee( const fee_type& fee, const string& op_name = "" );
   void validate_percentage( uint16_t percent, const string& object_name = "percentage" );
   void validate_positive_amount( share_type amount, const string& object_name = "amount" );
   void validate_non_negative_amount( share_type amount, const string& object_name = "amount" );
   void validate_positive_core_asset( const asset& a, const string& object_name = "amount" );
   void validate_non_negative_core_asset( const asset& a, const string& object_name = "amount" );
   void validate_positive_asset( const asset& a, const string& object_name = "amount" );
   void validate_non_negative_asset( const asset& a, const string& object_name = "amount" );

   ///@}

} } // graphene::chain

FC_REFLECT_TYPENAME( graphene::chain::operation_result )
FC_REFLECT_TYPENAME( graphene::chain::future_extensions )
FC_REFLECT( graphene::chain::void_result, )
FC_REFLECT( graphene::chain::default_extension_type, )

FC_REFLECT_TYPENAME( graphene::chain::fee_options_type )
FC_REFLECT( graphene::chain::fee_extension_type,
            (from_balance)(from_prepaid)(from_csaf) )
FC_REFLECT( graphene::chain::fee_type, (total)(options) )
