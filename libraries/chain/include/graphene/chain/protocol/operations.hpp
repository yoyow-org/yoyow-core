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
#include <graphene/chain/protocol/account.hpp>
#include <graphene/chain/protocol/asset_ops.hpp>
#include <graphene/chain/protocol/committee_member.hpp>
#include <graphene/chain/protocol/content.hpp>
#include <graphene/chain/protocol/advertising.hpp>
#include <graphene/chain/protocol/custom_vote.hpp>
#include <graphene/chain/protocol/csaf.hpp>
#include <graphene/chain/protocol/proposal.hpp>
#include <graphene/chain/protocol/transfer.hpp>
#include <graphene/chain/protocol/witness.hpp>
#include <graphene/chain/protocol/balance_lock.hpp>
#include <graphene/chain/protocol/pledge_mining.hpp>
#include <graphene/chain/protocol/market.hpp>

namespace graphene { namespace chain {

   /**
    * @ingroup operations
    *
    * Defines the set of valid operations as a discriminated union type.
    */
   typedef fc::static_variant<
            transfer_operation,
            account_create_operation,
            account_manage_operation,
            account_update_auth_operation,
            account_update_key_operation,
            account_update_proxy_operation,
            csaf_collect_operation,
            csaf_lease_operation,
            committee_member_create_operation,
            committee_member_update_operation,
            committee_member_vote_update_operation,
            committee_proposal_create_operation,
            committee_proposal_update_operation,
            witness_create_operation,
            witness_update_operation,
            witness_vote_update_operation,
            witness_collect_pay_operation,
            witness_report_operation,
            post_operation,
            post_update_operation,
            platform_create_operation,
            platform_update_operation,
            platform_vote_update_operation,
            account_auth_platform_operation,
            account_cancel_auth_platform_operation,
            asset_create_operation,
            asset_update_operation,
            asset_issue_operation,
            asset_reserve_operation,
            asset_claim_fees_operation,
            override_transfer_operation,
            proposal_create_operation,
            proposal_update_operation,
            proposal_delete_operation,
            account_enable_allowed_assets_operation,
            account_update_allowed_assets_operation,
            // the operations below are not supported
            account_whitelist_operation,
            score_create_operation,
            reward_operation,
            reward_proxy_operation,
            buyout_operation,
            license_create_operation,
            advertising_create_operation,
            advertising_update_operation,
            advertising_buy_operation,
            advertising_confirm_operation,
            advertising_ransom_operation,
            custom_vote_create_operation,
            custom_vote_cast_operation,
            balance_lock_update_operation,
            pledge_mining_update_operation,
            pledge_bonus_collect_operation,
            limit_order_create_operation,
            limit_order_cancel_operation,
            fill_order_operation,  // VIRTUAL
            market_fee_collect_operation,
            score_bonus_collect_operation,
            beneficiary_assign_operation,
            benefit_collect_operation
         > operation;

   /// @} // operations group

   // TODO possible performance improvement by using another data structure other than flat_set, when the size is big
   void operation_get_required_uid_authorities( const operation& op,
                                            flat_set<account_uid_type>& owner_uids,
                                            flat_set<account_uid_type>& active_uids,
                                            flat_set<account_uid_type>& secondary_uids,
                                            vector<authority>&  other,
                                            bool	  enabled_hardfork);

   void operation_validate( const operation& op );

   /**
    *  @brief necessary to support nested operations inside the proposal_create_operation
    */
   struct op_wrapper
   {
      public:
         op_wrapper(const operation& op = operation()):op(op){}
         operation op;
   };

} } // graphene::chain

FC_REFLECT_TYPENAME( graphene::chain::operation )
FC_REFLECT( graphene::chain::op_wrapper, (op) )
