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
#include <graphene/chain/protocol/types.hpp>
#include <fc/smart_ref_fwd.hpp>

namespace graphene { namespace chain { struct fee_schedule; } }
/*
namespace fc {
   template<typename Stream, typename T> inline void pack( Stream& s, const graphene::chain::fee_schedule& value );
   template<typename Stream, typename T> inline void unpack( Stream& s, graphene::chain::fee_schedule& value );
} // namespace fc
*/

namespace graphene { namespace chain {

   typedef static_variant<>  parameter_extension; 
   struct chain_parameters
   {
      /** using a smart ref breaks the circular dependency created between operations and the fee schedule */
      smart_ref<fee_schedule> current_fees;                       ///< current schedule of fees
      uint8_t                 block_interval                      = GRAPHENE_DEFAULT_BLOCK_INTERVAL; ///< interval in seconds between blocks
      uint32_t                maintenance_interval                = GRAPHENE_DEFAULT_MAINTENANCE_INTERVAL; ///< interval in sections between blockchain maintenance events
      uint8_t                 maintenance_skip_slots              = GRAPHENE_DEFAULT_MAINTENANCE_SKIP_SLOTS; ///< number of block_intervals to skip at maintenance time
      uint32_t                committee_proposal_review_period    = GRAPHENE_DEFAULT_COMMITTEE_PROPOSAL_REVIEW_PERIOD_SEC; ///< minimum time in seconds that a proposed transaction requiring committee authority may not be signed, prior to expiration
      uint32_t                maximum_transaction_size            = GRAPHENE_DEFAULT_MAX_TRANSACTION_SIZE; ///< maximum allowable size in bytes for a transaction
      uint32_t                maximum_block_size                  = GRAPHENE_DEFAULT_MAX_BLOCK_SIZE; ///< maximum allowable size in bytes for a block
      uint32_t                maximum_time_until_expiration       = GRAPHENE_DEFAULT_MAX_TIME_UNTIL_EXPIRATION; ///< maximum lifetime in seconds for transactions to be valid, before expiring
      uint32_t                maximum_proposal_lifetime           = GRAPHENE_DEFAULT_MAX_PROPOSAL_LIFETIME_SEC; ///< maximum lifetime in seconds for proposed transactions to be kept, before expiring
      uint8_t                 maximum_asset_whitelist_authorities = GRAPHENE_DEFAULT_MAX_ASSET_WHITELIST_AUTHORITIES; ///< maximum number of accounts which an asset may list as authorities for its whitelist OR blacklist
      uint8_t                 maximum_asset_feed_publishers       = GRAPHENE_DEFAULT_MAX_ASSET_FEED_PUBLISHERS; ///< the maximum number of feed publishers for a given asset
      uint16_t                maximum_witness_count               = GRAPHENE_DEFAULT_MAX_WITNESSES; ///< maximum number of active witnesses
      uint16_t                maximum_committee_count             = GRAPHENE_DEFAULT_MAX_COMMITTEE; ///< maximum number of active committee_members
      uint16_t                maximum_authority_membership        = GRAPHENE_DEFAULT_MAX_AUTHORITY_MEMBERSHIP; ///< largest number of keys/accounts an authority can have
      uint16_t                reserve_percent_of_fee              = GRAPHENE_DEFAULT_BURN_PERCENT_OF_FEE; ///< the percentage of the network's allocation of a fee that is taken out of circulation
      uint16_t                network_percent_of_fee              = GRAPHENE_DEFAULT_NETWORK_PERCENT_OF_FEE; ///< percent of transaction fees paid to network
      uint16_t                lifetime_referrer_percent_of_fee    = GRAPHENE_DEFAULT_LIFETIME_REFERRER_PERCENT_OF_FEE; ///< percent of transaction fees paid to network
      uint32_t                cashback_vesting_period_seconds     = GRAPHENE_DEFAULT_CASHBACK_VESTING_PERIOD_SEC; ///< time after cashback rewards are accrued before they become liquid
      share_type              cashback_vesting_threshold          = GRAPHENE_DEFAULT_CASHBACK_VESTING_THRESHOLD; ///< the maximum cashback that can be received without vesting
      bool                    count_non_member_votes              = true; ///< set to false to restrict voting privlegages to member accounts
      bool                    allow_non_member_whitelists         = false; ///< true if non-member accounts may set whitelists and blacklists; false otherwise
      share_type              witness_pay_per_block               = GRAPHENE_DEFAULT_WITNESS_PAY_PER_BLOCK; ///< CORE to be allocated to witnesses (per block)
      uint32_t                witness_pay_vesting_seconds         = GRAPHENE_DEFAULT_WITNESS_PAY_VESTING_SECONDS; ///< vesting_seconds parameter for witness VBO's
      share_type              worker_budget_per_day               = GRAPHENE_DEFAULT_WORKER_BUDGET_PER_DAY; ///< CORE to be allocated to workers (per day)
      uint16_t                max_predicate_opcode                = GRAPHENE_DEFAULT_MAX_ASSERT_OPCODE; ///< predicate_opcode must be less than this number
      share_type              fee_liquidation_threshold           = GRAPHENE_DEFAULT_FEE_LIQUIDATION_THRESHOLD; ///< value in CORE at which accumulated fees in blockchain-issued market assets should be liquidated
      uint16_t                accounts_per_fee_scale              = GRAPHENE_DEFAULT_ACCOUNTS_PER_FEE_SCALE; ///< number of accounts between fee scalings
      uint8_t                 account_fee_scale_bitshifts         = GRAPHENE_DEFAULT_ACCOUNT_FEE_SCALE_BITSHIFTS; ///< number of times to left bitshift account registration fee at each scaling
      uint8_t                 max_authority_depth                 = GRAPHENE_MAX_SIG_CHECK_DEPTH;
      uint64_t                csaf_rate                           = GRAPHENE_DEFAULT_CSAF_RATE;
      share_type              max_csaf_per_account                = GRAPHENE_DEFAULT_MAX_CSAF_PER_ACCOUNT;
      uint64_t                csaf_accumulate_window              = GRAPHENE_DEFAULT_CSAF_ACCUMULATE_WINDOW;
      uint64_t                min_witness_pledge                  = GRAPHENE_DEFAULT_MIN_WITNESS_PLEDGE;
      uint64_t                max_witness_pledge_seconds          = GRAPHENE_DEFAULT_MAX_WITNESS_PLEDGE_SECONDS;
      uint32_t                witness_avg_pledge_update_interval  = GRAPHENE_DEFAULT_WITNESS_AVG_PLEDGE_UPDATE_INTERVAL;
      uint32_t                witness_pledge_release_delay        = GRAPHENE_DEFAULT_WITNESS_PLEDGE_RELEASE_DELAY;
      uint64_t                min_governance_voting_balance       = GRAPHENE_DEFAULT_MIN_GOVERNANCE_VOTING_BALANCE;
      uint8_t                 max_governance_voting_proxy_level   = GRAPHENE_DEFAULT_MAX_GOVERNANCE_VOTING_RPOXY_LEVEL;
      uint32_t                governance_voting_expiration_blocks = GRAPHENE_DEFAULT_GOVERNANCE_VOTING_EXPIRATION_BLOCKS;
      uint32_t                governance_votes_update_interval    = GRAPHENE_DEFAULT_GOVERNANCE_VOTES_UPDATE_INTERVAL;
      uint64_t                max_governance_votes_seconds        = GRAPHENE_DEFAULT_MAX_GOVERNANCE_VOTES_SECONDS;
      uint16_t                max_witnesses_voted_per_account     = GRAPHENE_DEFAULT_MAX_WITNESSES_VOTED_PER_ACCOUNT;
      uint32_t                max_witness_inactive_blocks         = GRAPHENE_DEFAULT_MAX_WITNESS_INACTIVE_BLOCKS;
      share_type              by_vote_top_witness_pay_per_block   = GRAPHENE_DEFAULT_BY_VOTE_TOP_WITNESS_PAY_PER_BLOCK;
      share_type              by_vote_rest_witness_pay_per_block  = GRAPHENE_DEFAULT_BY_VOTE_REST_WITNESS_PAY_PER_BLOCK;
      share_type              by_pledge_witness_pay_per_block     = GRAPHENE_DEFAULT_BY_PLEDGE_WITNESS_PAY_PER_BLOCK;
      uint16_t                by_vote_top_witness_count           = GRAPHENE_DEFAULT_BY_VOTE_TOP_WITNESSES;
      uint16_t                by_vote_rest_witness_count          = GRAPHENE_DEFAULT_BY_VOTE_REST_WITNESSES;
      uint16_t                by_pledge_witness_count             = GRAPHENE_DEFAULT_BY_PLEDGE_WITNESSES;
      uint32_t                budget_adjust_interval              = GRAPHENE_DEFAULT_BUDGET_ADJUST_INTERVAL;
      uint16_t                budget_adjust_target                = GRAPHENE_DEFAULT_BUDGET_ADJUST_TARGET;
      uint8_t                 committee_size                      = GRAPHENE_DEFAULT_COMMITTEE_SIZE;
      uint32_t                committee_update_interval           = GRAPHENE_DEFAULT_COMMITTEE_UPDATE_INTERVAL;
      uint64_t                min_committee_member_pledge         = GRAPHENE_DEFAULT_MIN_COMMITTEE_MEMBER_PLEDGE;
      uint32_t                committee_member_pledge_release_delay   = GRAPHENE_DEFAULT_COMMITTEE_MEMBER_PLEDGE_RELEASE_DELAY;
      uint16_t                max_committee_members_voted_per_account = GRAPHENE_DEFAULT_MAX_COMMITTEE_MEMBERS_VOTED_PER_ACCOUNT;
      uint32_t                witness_report_prosecution_period       = GRAPHENE_DEFAULT_WITNESS_REPORT_PROSECUTION_PERIOD;
      bool                    witness_report_allow_pre_last_block     = GRAPHENE_DEFAULT_WITNESS_REPORT_ALLOW_PRE_LAST_BLOCK;
      share_type              witness_report_pledge_deduction_amount  = GRAPHENE_DEFAULT_WITNESS_REPORT_PLEDGE_DEDUCTION_AMOUNT;
      uint64_t                platform_min_pledge                     = GRAPHENE_DEFAULT_PLATFORM_MIN_PLEDGE;
      uint32_t                platform_pledge_release_delay           = GRAPHENE_DEFAULT_PLATFORM_PLEDGE_RELEASE_DELAY;
      uint8_t                 platform_max_vote_per_account           = GRAPHENE_DEFAULT_PLATFORM_MAX_VOTE_PER_ACCOUNT;
      extensions_type         extensions;

      /** defined in fee_schedule.cpp */
      void validate()const;
   };

} }  // graphene::chain

FC_REFLECT( graphene::chain::chain_parameters,
            (current_fees)
            (block_interval)
            //(maintenance_interval)
            //(maintenance_skip_slots)
            //(committee_proposal_review_period)
            (maximum_transaction_size)
            (maximum_block_size)
            (maximum_time_until_expiration)
            //(maximum_proposal_lifetime)
            //(maximum_asset_whitelist_authorities)
            //(maximum_asset_feed_publishers)
            //(maximum_witness_count)
            //(maximum_committee_count)
            (maximum_authority_membership)
            //(reserve_percent_of_fee)
            //(network_percent_of_fee)
            //(lifetime_referrer_percent_of_fee)
            //(cashback_vesting_period_seconds)
            //(cashback_vesting_threshold)
            //(count_non_member_votes)
            //(allow_non_member_whitelists)
            //(witness_pay_per_block)
            //(worker_budget_per_day)
            //(max_predicate_opcode)
            //(fee_liquidation_threshold)
            //(accounts_per_fee_scale)
            //(account_fee_scale_bitshifts)
            (max_authority_depth)
            (csaf_rate)
            (max_csaf_per_account)
            (csaf_accumulate_window)
            (min_witness_pledge)
            (max_witness_pledge_seconds)
            (witness_avg_pledge_update_interval)
            (witness_pledge_release_delay)
            (min_governance_voting_balance)
            (max_governance_voting_proxy_level)
            (governance_voting_expiration_blocks)
            (governance_votes_update_interval)
            (max_governance_votes_seconds)
            (max_witnesses_voted_per_account)
            (max_witness_inactive_blocks)
            (by_vote_top_witness_pay_per_block)
            (by_vote_rest_witness_pay_per_block)
            (by_pledge_witness_pay_per_block)
            (by_vote_top_witness_count)
            (by_vote_rest_witness_count)
            (by_pledge_witness_count)
            (budget_adjust_interval)
            (budget_adjust_target)
            (committee_size)
            (committee_update_interval)
            (min_committee_member_pledge)
            (committee_member_pledge_release_delay)
            (max_committee_members_voted_per_account)
            (witness_report_prosecution_period)
            (witness_report_allow_pre_last_block)
            (witness_report_pledge_deduction_amount)
            (platform_min_pledge)
            (platform_pledge_release_delay)(platform_max_vote_per_account)(extensions)
          )
