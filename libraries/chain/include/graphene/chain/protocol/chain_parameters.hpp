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

	 struct content_parameter_extension_type
	 {
      uint32_t    content_award_interval              = GRAPHENE_DEFAULT_CONTENT_AWARD_INTERVAL;///< interval in seconds between content awards
      uint32_t    platform_award_interval             = GRAPHENE_DEFAULT_PLATFORM_AWARD_INTERVAL;///< interval in seconds between platform vote awards
      share_type  max_csaf_per_approval               = GRAPHENE_DEFAULT_MAX_CSAF_PER_APPROVAL;///< score behavior spend csaf must less than or equal to this number
      uint32_t    approval_expiration                 = GRAPHENE_DEFAULT_APPROVAL_EXPIRATION;///< maximum lifetime in seconds for score, after same account can again score for same post 
      share_type  min_effective_csaf                  = GRAPHENE_DEFAULT_MIN_EFFECTIVE_CSAF;///< minimum csaf required for post in content awards 
      share_type  total_content_award_amount          = GRAPHENE_DEFAULT_TOTAL_CONTENT_AWARD_AMOUNT;///< total content award amount, per year 
      share_type  total_platform_content_award_amount = GRAPHENE_DEFAULT_TOTAL_PLATFORM_CONTENT_AWARD_AMOUNT;///< total platform award content amount, per year
      share_type  total_platform_voted_award_amount	= GRAPHENE_DEFAULT_TOTAL_PLATFORM_VOTED_AWARD_AMOUNT;///< total platform vote award amount, per year
      share_type  platform_award_min_votes            = GRAPHENE_DEFAULT_PLATFORM_AWARD_MIN_VOTES;///< minimum votes required for platform in platform vote awards
      uint32_t    platform_award_requested_rank       = GRAPHENE_DEFAULT_PLATFORM_AWARD_REQUESTED_RANK;///< minimum votes rank required for platform in platform vote awards
      
      uint32_t    platform_award_basic_rate           = GRAPHENE_DEFAULT_PLATFORM_AWARD_BASIC_RATE;///< in platform vote awards, average allocation of this part, remaining awards distribute to platform by votes
      uint32_t    casf_modulus                        = GRAPHENE_DEFAULT_CASF_MODULUS;///< compute effective csaf modulus
      uint32_t    post_award_expiration               = GRAPHENE_DEFAULT_POST_AWARD_EXPIRATION;///< if current time greater than this time, post can't get post award
      uint32_t    approval_casf_min_weight            = GRAPHENE_DEFAULT_APPROVAL_MIN_CASF_WEIGHT;///< the minimum percentage of csaf, when you compute the effective csaf
      uint32_t    approval_casf_first_rate            = GRAPHENE_DEFAULT_APPROVAL_CASF_FIRST_RATE;///< the first percentage of csaf, when you compute the effective csaf
      uint32_t    approval_casf_second_rate           = GRAPHENE_DEFAULT_APPROVAL_CASF_SECOND_RATE;///< the second percentage of csaf, when you compute the effective csaf, greater than the first percentage
      uint32_t    receiptor_award_modulus             = GRAPHENE_DEFAULT_RECEIPTOR_AWARD_MODULUS;///< when post of disapprove more than approve, receiptor award multiply this number, this number must less than 100%
      uint32_t    disapprove_award_modulus            = GRAPHENE_DEFAULT_DISAPPROVE_AWARD_MODULUS;///< when post of disapprove more than approve, scorer award multiply this number, this number must more than 100%
               
      uint32_t    advertising_confirmed_fee_rate      = GRAPHENE_DEFAULT_ADVERTISING_CONFIRMED_FEE_RATE;///< fee rate of advertising order price
      share_type  advertising_confirmed_min_fee       = GRAPHENE_DEFAULT_ADVERTISING_CONFIRMED_MIN_FEE;///< confirm advertising order minimum fee,that return to capital pool
      uint32_t    custom_vote_effective_time          = GRAPHENE_DEFAULT_CUSTOM_VOTE_EFFECTIVE_TIME;///< custom vote effective time, if more than this time, clear custom vote object

      uint64_t    min_witness_block_produce_pledge    = GRAPHENE_DEFAULT_MIN_WITNESS_BLOCK_PRODUCE_PLEDGE;///< pos, witness pledge must greater this number, can produce block
      uint8_t		content_award_skip_slots		      = 0;
      uint32_t    unlocked_balance_release_delay      = GRAPHENE_DEFAULT_UNLOCKED_BALANCE_RELEASE_DELAY;
	 };

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
      bool                    allow_non_member_whitelists         = true; ///< true if non-member accounts may set whitelists and blacklists; false otherwise
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
      uint64_t                platform_max_pledge_seconds             = GRAPHENE_DEFAULT_PLATFORM_MAX_PLEDGE_SECONDS;
      uint32_t                platform_avg_pledge_update_interval     = GRAPHENE_DEFAULT_PLATFORM_AVG_PLEDGE_UPDATE_INTERVAL;
      uint32_t                platform_pledge_release_delay           = GRAPHENE_DEFAULT_PLATFORM_PLEDGE_RELEASE_DELAY;
      uint16_t                platform_max_vote_per_account           = GRAPHENE_DEFAULT_PLATFORM_MAX_VOTE_PER_ACCOUNT;
		
      content_parameter_extension_type content_parameters;

      /** defined in fee_schedule.cpp */
      void validate()const;

      content_parameter_extension_type get_award_params()const { return content_parameters; }
   };

} }  // graphene::chain
FC_REFLECT(	graphene::chain::content_parameter_extension_type, 
   (content_award_interval)
   (platform_award_interval)
   (max_csaf_per_approval)
   (approval_expiration)
   (min_effective_csaf)
   (total_content_award_amount)
   (total_platform_content_award_amount)
   (total_platform_voted_award_amount)
   (platform_award_min_votes)
   (platform_award_requested_rank)
   (platform_award_basic_rate)
   (casf_modulus)
   (post_award_expiration)
   (approval_casf_min_weight)
   (approval_casf_first_rate)
   (approval_casf_second_rate)
   (receiptor_award_modulus)
   (disapprove_award_modulus)
   (advertising_confirmed_fee_rate)
   (advertising_confirmed_min_fee)
   (custom_vote_effective_time)
   (min_witness_block_produce_pledge)
   (content_award_skip_slots)
   (unlocked_balance_release_delay))

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
            (platform_max_pledge_seconds)
            (platform_avg_pledge_update_interval)
            (platform_pledge_release_delay)
            (platform_max_vote_per_account)
            (content_parameters)
          )
