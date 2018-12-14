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

#define GRAPHENE_SYMBOL "YOYO"
#define GRAPHENE_ADDRESS_PREFIX "YYW"

#define GRAPHENE_MIN_ACCOUNT_NAME_LENGTH 2
#define GRAPHENE_MAX_ACCOUNT_NAME_LENGTH 63

#define GRAPHENE_MAX_PLATFORM_NAME_LENGTH 100
#define GRAPHENE_MAX_PLATFORM_EXTRA_DATA_LENGTH 1000

#define GRAPHENE_MIN_ASSET_SYMBOL_LENGTH 3
#define GRAPHENE_MAX_ASSET_SYMBOL_LENGTH 16

#define GRAPHENE_MAX_SHARE_SUPPLY int64_t(1000000000000000ll)
#define GRAPHENE_MAX_PAY_RATE 10000 /* 100% */
#define GRAPHENE_MAX_SIG_CHECK_DEPTH 2
/**
 * Don't allow the committee_members to publish a limit that would
 * make the network unable to operate.
 */
#define GRAPHENE_MIN_TRANSACTION_SIZE_LIMIT 1024
#define GRAPHENE_MIN_BLOCK_INTERVAL   1 /* seconds */
#define GRAPHENE_MAX_BLOCK_INTERVAL  30 /* seconds */

#define GRAPHENE_DEFAULT_BLOCK_INTERVAL  3 /* seconds */
#define GRAPHENE_DEFAULT_MAX_TRANSACTION_SIZE 65536
#define GRAPHENE_DEFAULT_MAX_BLOCK_SIZE  (GRAPHENE_DEFAULT_MAX_TRANSACTION_SIZE*16*GRAPHENE_DEFAULT_BLOCK_INTERVAL)
#define GRAPHENE_DEFAULT_MAX_TIME_UNTIL_EXPIRATION (60*60*24) // seconds,  aka: 1 day
#define GRAPHENE_DEFAULT_MAINTENANCE_INTERVAL  (60*60*24) // seconds, aka: 1 day
#define GRAPHENE_DEFAULT_MAINTENANCE_SKIP_SLOTS 0  // number of slots to skip for maintenance interval

#define GRAPHENE_MIN_UNDO_HISTORY 10
#define GRAPHENE_MAX_UNDO_HISTORY 10000

#define GRAPHENE_MIN_BLOCK_SIZE_LIMIT (GRAPHENE_MIN_TRANSACTION_SIZE_LIMIT*5) // 5 transactions per block
#define GRAPHENE_MIN_TRANSACTION_EXPIRATION_LIMIT (GRAPHENE_MAX_BLOCK_INTERVAL * 5) // 5 transactions per block
#define GRAPHENE_BLOCKCHAIN_PRECISION                           uint64_t( 100000 )

#define GRAPHENE_BLOCKCHAIN_PRECISION_DIGITS                    5
#define GRAPHENE_DEFAULT_TRANSFER_FEE                           (1*GRAPHENE_BLOCKCHAIN_PRECISION)
#define GRAPHENE_MAX_INSTANCE_ID                                (uint64_t(-1)>>16)
/** percentage fields are fixed point with a denominator of 10,000 */
#define GRAPHENE_100_PERCENT                                    10000
#define GRAPHENE_1_PERCENT                                      (GRAPHENE_100_PERCENT/100)
/** NOTE: making this a power of 2 (say 2^15) would greatly accelerate fee calcs */
#define GRAPHENE_MAX_MARKET_FEE_PERCENT                         GRAPHENE_100_PERCENT
#define GRAPHENE_DEFAULT_FORCE_SETTLEMENT_DELAY                 (60*60*24) ///< 1 day
#define GRAPHENE_DEFAULT_FORCE_SETTLEMENT_OFFSET                0 ///< 1%
#define GRAPHENE_DEFAULT_FORCE_SETTLEMENT_MAX_VOLUME            (20* GRAPHENE_1_PERCENT) ///< 20%
#define GRAPHENE_DEFAULT_PRICE_FEED_LIFETIME                    (60*60*24) ///< 1 day
#define GRAPHENE_MAX_FEED_PRODUCERS                             200
#define GRAPHENE_DEFAULT_MAX_AUTHORITY_MEMBERSHIP               10
#define GRAPHENE_DEFAULT_MAX_ASSET_WHITELIST_AUTHORITIES        10
#define GRAPHENE_DEFAULT_MAX_ASSET_FEED_PUBLISHERS              10

/**
 *  These ratios are fixed point numbers with a denominator of GRAPHENE_COLLATERAL_RATIO_DENOM, the
 *  minimum maitenance collateral is therefore 1.001x and the default
 *  maintenance ratio is 1.75x
 */
///@{
#define GRAPHENE_COLLATERAL_RATIO_DENOM                 1000
#define GRAPHENE_MIN_COLLATERAL_RATIO                   1001  ///< lower than this could result in divide by 0
#define GRAPHENE_MAX_COLLATERAL_RATIO                   32000 ///< higher than this is unnecessary and may exceed int16 storage
#define GRAPHENE_DEFAULT_MAINTENANCE_COLLATERAL_RATIO   1750 ///< Call when collateral only pays off 175% the debt
#define GRAPHENE_DEFAULT_MAX_SHORT_SQUEEZE_RATIO        1500 ///< Stop calling when collateral only pays off 150% of the debt
///@}
#define GRAPHENE_DEFAULT_MARGIN_PERIOD_SEC              (30*60*60*24)

#define GRAPHENE_DEFAULT_MIN_WITNESS_COUNT                    (11)
#define GRAPHENE_DEFAULT_MIN_COMMITTEE_MEMBER_COUNT           (11)
#define GRAPHENE_DEFAULT_MAX_WITNESSES                        (1001) // SHOULD BE ODD
#define GRAPHENE_DEFAULT_MAX_COMMITTEE                        (1001) // SHOULD BE ODD
#define GRAPHENE_DEFAULT_MAX_PROPOSAL_LIFETIME_SEC            (60*60*24*7*4) // Four weeks
#define GRAPHENE_DEFAULT_COMMITTEE_PROPOSAL_REVIEW_PERIOD_SEC (60*60*24*7*2) // Two weeks
#define GRAPHENE_DEFAULT_NETWORK_PERCENT_OF_FEE               (20*GRAPHENE_1_PERCENT)
#define GRAPHENE_DEFAULT_LIFETIME_REFERRER_PERCENT_OF_FEE     (30*GRAPHENE_1_PERCENT)
#define GRAPHENE_DEFAULT_MAX_BULK_DISCOUNT_PERCENT            (50*GRAPHENE_1_PERCENT)
#define GRAPHENE_DEFAULT_BULK_DISCOUNT_THRESHOLD_MIN          ( GRAPHENE_BLOCKCHAIN_PRECISION*int64_t(1000) )
#define GRAPHENE_DEFAULT_BULK_DISCOUNT_THRESHOLD_MAX          ( GRAPHENE_DEFAULT_BULK_DISCOUNT_THRESHOLD_MIN*int64_t(100) )
#define GRAPHENE_DEFAULT_CASHBACK_VESTING_PERIOD_SEC          (60*60*24*365) ///< 1 year
#define GRAPHENE_DEFAULT_CASHBACK_VESTING_THRESHOLD           (GRAPHENE_BLOCKCHAIN_PRECISION*int64_t(100))
#define GRAPHENE_DEFAULT_BURN_PERCENT_OF_FEE                  (20*GRAPHENE_1_PERCENT)
#define GRAPHENE_WITNESS_PAY_PERCENT_PRECISION                (1000000000)
#define GRAPHENE_DEFAULT_MAX_ASSERT_OPCODE                    1
#define GRAPHENE_DEFAULT_FEE_LIQUIDATION_THRESHOLD            GRAPHENE_BLOCKCHAIN_PRECISION * 100;
#define GRAPHENE_DEFAULT_ACCOUNTS_PER_FEE_SCALE               1000
#define GRAPHENE_DEFAULT_ACCOUNT_FEE_SCALE_BITSHIFTS          0
#define GRAPHENE_DEFAULT_MAX_BUYBACK_MARKETS                  4

#define GRAPHENE_MAX_WORKER_NAME_LENGTH                       63

#define GRAPHENE_MAX_URL_LENGTH                               127

#define GRAPHENE_CORE_ASSET_AID                               (uint64_t(0))

#define GRAPHENE_DEFAULT_CSAF_RATE                            (uint64_t(86400) * 10000)
#define GRAPHENE_DEFAULT_MAX_CSAF_PER_ACCOUNT                 (GRAPHENE_BLOCKCHAIN_PRECISION * int64_t(1000))
#define GRAPHENE_DEFAULT_CSAF_ACCUMULATE_WINDOW               (60*60*24*7) // One week

#define GRAPHENE_DEFAULT_MIN_WITNESS_PLEDGE                   (GRAPHENE_BLOCKCHAIN_PRECISION * int64_t(10000))
/*
//for test. change back when use new genesis
#define GRAPHENE_DEFAULT_MAX_WITNESS_PLEDGE_SECONDS           (60*30) // 30 minutes
#define GRAPHENE_DEFAULT_WITNESS_AVG_PLEDGE_UPDATE_INTERVAL   100  // blocks,  5 minutes if 3 seconds per block
#define GRAPHENE_DEFAULT_WITNESS_PLEDGE_RELEASE_DELAY         300  // blocks, 15 minutes if 3 seconds per block
#define GRAPHENE_DEFAULT_BUDGET_ADJUST_INTERVAL               (28800*3) // blocks, 3 days if 3 seconds per block
#define GRAPHENE_DEFAULT_GOVERNANCE_VOTING_EXPIRATION_BLOCKS  (28800*5) // blocks, 5 days if 3 seconds per block
#define GRAPHENE_DEFAULT_MAX_GOVERNANCE_VOTES_SECONDS         (60*60*24*4) // 4 days
#define GRAPHENE_DEFAULT_COMMITTEE_UPDATE_INTERVAL               (1200) // blocks, 1 hour if 3 seconds per block
*/
#define GRAPHENE_DEFAULT_MAX_WITNESS_PLEDGE_SECONDS           (60*60*24*7) // One week
#define GRAPHENE_DEFAULT_WITNESS_AVG_PLEDGE_UPDATE_INTERVAL   1200  // blocks, one hour if 3 seconds per block
#define GRAPHENE_DEFAULT_WITNESS_PLEDGE_RELEASE_DELAY         28800 // blocks, one day if 3 seconds per block
#define GRAPHENE_DEFAULT_MIN_GOVERNANCE_VOTING_BALANCE        (GRAPHENE_BLOCKCHAIN_PRECISION * uint64_t(10000))
#define GRAPHENE_DEFAULT_MAX_GOVERNANCE_VOTING_RPOXY_LEVEL    (4)
#define GRAPHENE_DEFAULT_GOVERNANCE_VOTING_EXPIRATION_BLOCKS  (28800*90) // blocks, 90 days if 3 seconds per block
#define GRAPHENE_DEFAULT_GOVERNANCE_VOTES_UPDATE_INTERVAL     (28800) // blocks, 1 day if 3 seconds per block
#define GRAPHENE_DEFAULT_MAX_GOVERNANCE_VOTES_SECONDS         (60*60*24*60) // 60 days
#define GRAPHENE_DEFAULT_MAX_WITNESSES_VOTED_PER_ACCOUNT      (101)
#define GRAPHENE_DEFAULT_MAX_WITNESS_INACTIVE_BLOCKS          (28800) // blocks, 1 day if 3 seconds per block

#define GRAPHENE_DEFAULT_BY_VOTE_TOP_WITNESS_PAY_PER_BLOCK    (GRAPHENE_BLOCKCHAIN_PRECISION * uint64_t(1))
#define GRAPHENE_DEFAULT_BY_VOTE_REST_WITNESS_PAY_PER_BLOCK   (GRAPHENE_BLOCKCHAIN_PRECISION * uint64_t(1))
#define GRAPHENE_DEFAULT_BY_PLEDGE_WITNESS_PAY_PER_BLOCK      (GRAPHENE_BLOCKCHAIN_PRECISION * uint64_t(1) / 2)
#define GRAPHENE_DEFAULT_BY_VOTE_TOP_WITNESSES                (9)
#define GRAPHENE_DEFAULT_BY_VOTE_REST_WITNESSES               (1)
#define GRAPHENE_DEFAULT_BY_PLEDGE_WITNESSES                  (1)

#define GRAPHENE_DEFAULT_BUDGET_ADJUST_INTERVAL               (28800*365) // blocks, 1 year if 3 seconds per block
#define GRAPHENE_DEFAULT_BUDGET_ADJUST_TARGET                 (10 * GRAPHENE_1_PERCENT) // (max_supply - current_supply) * x%

#define GRAPHENE_DEFAULT_COMMITTEE_SIZE                          (5) // number of active committee members
#define GRAPHENE_DEFAULT_COMMITTEE_UPDATE_INTERVAL               (28800*30) // blocks, 30 days if 3 seconds per block
#define GRAPHENE_DEFAULT_MIN_COMMITTEE_MEMBER_PLEDGE             (GRAPHENE_BLOCKCHAIN_PRECISION * uint64_t(1000))
#define GRAPHENE_DEFAULT_COMMITTEE_MEMBER_PLEDGE_RELEASE_DELAY   (28800) // blocks, 1 day if 3 seconds per block
#define GRAPHENE_DEFAULT_MAX_COMMITTEE_MEMBERS_VOTED_PER_ACCOUNT (1)

#define GRAPHENE_DEFAULT_WITNESS_REPORT_PROSECUTION_PERIOD       (28800) // blocks, 1 day if 3 seconds per block
#define GRAPHENE_DEFAULT_WITNESS_REPORT_ALLOW_PRE_LAST_BLOCK     (false) // don't allow reporting of blocks earlier than last block
#define GRAPHENE_DEFAULT_WITNESS_REPORT_PLEDGE_DEDUCTION_AMOUNT  (GRAPHENE_BLOCKCHAIN_PRECISION * uint64_t(1000))

//Constants
#define GRAPHENE_VIRTUAL_LAP_LENGTH                           (fc::uint128_t::max_value())
#define GRAPHENE_MIN_GOVERNANCE_VOTING_EXPIRATION_BLOCKS      (28800) // blocks, 1 day if 3 seconds per block
#define GRAPHENE_MAX_EXPIRED_VOTERS_TO_PROCESS_PER_BLOCK      (10000)
#define GRAPHENE_MAX_RESIGNED_WITNESS_VOTES_PER_BLOCK         (10000)
#define GRAPHENE_MAX_RESIGNED_COMMITTEE_VOTES_PER_BLOCK       (10000)
#define GRAPHENE_MAX_CSAF_COLLECTING_TIME_OFFSET              (300) // 5 minutes
#define GRAPHENE_MAX_RESIGNED_PLATFORM_VOTES_PER_BLOCK        (10000)

// committee proposal pass thresholds
#define GRAPHENE_CPPT_FEE_DEFAULT                                 (uint16_t(5001)) // 50.01%
#define GRAPHENE_CPPT_FEE_COMMITTEE_MEMBER_CREATE_OP              (uint16_t(8500))
#define GRAPHENE_CPPT_ACCOUNT_CAN_VOTE                            (uint16_t(6500))
#define GRAPHENE_CPPT_ACCOUNT_IS_ADMIN                            (uint16_t(5001))
#define GRAPHENE_CPPT_ACCOUNT_IS_REGISTRAR                        (uint16_t(6500))
#define GRAPHENE_CPPT_ACCOUNT_TAKEOVER_REGISTRAR                  (uint16_t(6500))
#define GRAPHENE_CPPT_PARAM_MAX_TRX_SIZE                          (uint16_t(5001))
#define GRAPHENE_CPPT_PARAM_MAX_BLOCK_SIZE                        (uint16_t(5001))
#define GRAPHENE_CPPT_PARAM_MAX_EXPIRATION_TIME                   (uint16_t(5001))
#define GRAPHENE_CPPT_PARAM_MAX_AUTHORITY_MEMBERSHIP              (uint16_t(7500))
#define GRAPHENE_CPPT_PARAM_MAX_AUTHORITY_DEPTH                   (uint16_t(7500))
#define GRAPHENE_CPPT_PARAM_CSAF_RATE                             (uint16_t(5001))
#define GRAPHENE_CPPT_PARAM_MAX_CSAF_PER_ACCOUNT                  (uint16_t(5001))
#define GRAPHENE_CPPT_PARAM_CSAF_ACCUMULATE_WINDOW                (uint16_t(5001))
#define GRAPHENE_CPPT_PARAM_MIN_WITNESS_PLEDGE                    (uint16_t(6500))
#define GRAPHENE_CPPT_PARAM_MAX_WITNESS_PLEDGE_SECONDS            (uint16_t(6500))
#define GRAPHENE_CPPT_PARAM_AVG_WITNESS_PLEDGE_UPDATE_INTERVAL    (uint16_t(5001))
#define GRAPHENE_CPPT_PARAM_WITNESS_PLEDGE_RELEASE_DELAY          (uint16_t(5001))
#define GRAPHENE_CPPT_PARAM_MIN_GOVERNANCE_VOTING_BALANCE         (uint16_t(6500))
#define GRAPHENE_CPPT_PARAM_GOVERNANCE_VOTING_EXPIRATION_BLOCKS   (uint16_t(6500))
#define GRAPHENE_CPPT_PARAM_GOVERNANCE_VOTES_UPDATE_INTERVAL      (uint16_t(5001))
#define GRAPHENE_CPPT_PARAM_MAX_GOVERNANCE_VOTES_SECONDS          (uint16_t(6500))
#define GRAPHENE_CPPT_PARAM_MAX_WITNESSES_VOTED_PER_ACCOUNT       (uint16_t(6500))
#define GRAPHENE_CPPT_PARAM_MAX_WITNESS_INACTIVE_BLOCKS           (uint16_t(6500))
#define GRAPHENE_CPPT_PARAM_BY_VOTE_TOP_WITNESS_PAY_PER_BLOCK     (uint16_t(6500))
#define GRAPHENE_CPPT_PARAM_BY_VOTE_REST_WITNESS_PAY_PER_BLOCK    (uint16_t(6500))
#define GRAPHENE_CPPT_PARAM_BY_PLEDGE_WITNESS_PAY_PER_BLOCK       (uint16_t(6500))
#define GRAPHENE_CPPT_PARAM_BY_VOTE_TOP_WITNESS_COUNT             (uint16_t(6500))
#define GRAPHENE_CPPT_PARAM_BY_VOTE_REST_WITNESS_COUNT            (uint16_t(6500))
#define GRAPHENE_CPPT_PARAM_BY_PLEDGE_WITNESS_COUNT               (uint16_t(6500))
#define GRAPHENE_CPPT_PARAM_BUDGET_ADJUST_INTERVAL                (uint16_t(8500))
#define GRAPHENE_CPPT_PARAM_BUDGET_ADJUST_TARGET                  (uint16_t(8500))
#define GRAPHENE_CPPT_PARAM_MIN_COMMITTEE_MEMBER_PLEDGE           (uint16_t(8500))
#define GRAPHENE_CPPT_PARAM_COMMITTEE_MEMBER_PLEDGE_RELEASE_DELAY (uint16_t(8500))
#define GRAPHENE_CPPT_PARAM_WITNESS_REPORT_PROSECUTION_PERIOD     (uint16_t(6500))
#define GRAPHENE_CPPT_PARAM_WITNESS_REPORT_ALLOW_PRE_LAST_BLOCK   (uint16_t(6500))
#define GRAPHENE_CPPT_PARAM_WITNESS_REPORT_PLEDGE_DEDUCTION_AMOUNT (uint16_t(6500))

#define GRAPHENE_CPPT_PARAM_PLATFORM_MIN_PLEDGE                    (uint16_t(6500))
#define GRAPHENE_CPPT_PARAM_PLATFORM_PLEDGE_RELEASE_DELAY          (uint16_t(6500))
#define GRAPHENE_CPPT_PARAM_PLATFORM_MAX_VOTE_PER_ACCOUNT          (uint16_t(6500))
#define GRAPHENE_CPPT_PARAM_PLATFORM_MAX_PLEDGE_SECONDS            (uint16_t(6500))
#define GRAPHENE_CPPT_PARAM_PLATFORM_AVG_PLEDGE_UPDATE_INTERVAL    (uint16_t(6500))


// counter initialization values used to derive near and far future seeds for shuffling witnesses
// we use the fractional bits of sqrt(2) in hex
#define GRAPHENE_NEAR_SCHEDULE_CTR_IV                    ( (uint64_t( 0x6a09 ) << 0x30)    \
                                                         | (uint64_t( 0xe667 ) << 0x20)    \
                                                         | (uint64_t( 0xf3bc ) << 0x10)    \
                                                         | (uint64_t( 0xc908 )        ) )

// and the fractional bits of sqrt(3) in hex
#define GRAPHENE_FAR_SCHEDULE_CTR_IV                     ( (uint64_t( 0xbb67 ) << 0x30)    \
                                                         | (uint64_t( 0xae85 ) << 0x20)    \
                                                         | (uint64_t( 0x84ca ) << 0x10)    \
                                                         | (uint64_t( 0xa73b )        ) )

/**
 * every second, the fraction of burned core asset which cycles is
 * GRAPHENE_CORE_ASSET_CYCLE_RATE / (1 << GRAPHENE_CORE_ASSET_CYCLE_RATE_BITS)
 */
#define GRAPHENE_CORE_ASSET_CYCLE_RATE                        17
#define GRAPHENE_CORE_ASSET_CYCLE_RATE_BITS                   32

#define GRAPHENE_DEFAULT_WITNESS_PAY_PER_BLOCK            (GRAPHENE_BLOCKCHAIN_PRECISION * int64_t( 10) )
#define GRAPHENE_DEFAULT_WITNESS_PAY_VESTING_SECONDS      (60*60*24)
#define GRAPHENE_DEFAULT_WORKER_BUDGET_PER_DAY            (GRAPHENE_BLOCKCHAIN_PRECISION * int64_t(500) * 1000 )

#define GRAPHENE_DEFAULT_MINIMUM_FEEDS                       7

#define GRAPHENE_MAX_INTEREST_APR                            uint16_t( 10000 )

#define GRAPHENE_RECENTLY_MISSED_COUNT_INCREMENT             4
#define GRAPHENE_RECENTLY_MISSED_COUNT_DECREMENT             3

#define GRAPHENE_CURRENT_DB_VERSION                          "YYW1.1"

#define GRAPHENE_IRREVERSIBLE_THRESHOLD                      (75 * GRAPHENE_1_PERCENT)

/**
 *  Reserved Account IDs with special meaning
 */
///@{
/// Represents the canonical account for specifying you will vote directly (as opposed to a proxy)
#define GRAPHENE_PROXY_TO_SELF_ACCOUNT (graphene::chain::account_id_type(0))
/// Represents the canonical account for specifying you will vote directly (as opposed to a proxy)
#define GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID (graphene::chain::calc_account_uid(0))
/// Represents the current committee members, two-week review period
#define GRAPHENE_COMMITTEE_ACCOUNT (graphene::chain::account_id_type(1))
/// Represents the current committee members, two-week review period
#define GRAPHENE_COMMITTEE_ACCOUNT_UID (graphene::chain::calc_account_uid(1))
/// Represents the current witnesses
#define GRAPHENE_WITNESS_ACCOUNT (graphene::chain::account_id_type(2))
/// Represents the current witnesses
#define GRAPHENE_WITNESS_ACCOUNT_UID (graphene::chain::calc_account_uid(2))
/// Represents the current committee members
#define GRAPHENE_RELAXED_COMMITTEE_ACCOUNT (graphene::chain::account_id_type(3))
/// Represents the current committee members
#define GRAPHENE_RELAXED_COMMITTEE_ACCOUNT_UID (graphene::chain::calc_account_uid(3))
/// Represents the canonical account with NO authority (nobody can access funds in null account)
#define GRAPHENE_NULL_ACCOUNT (graphene::chain::account_id_type(4))
/// Represents the canonical account with NO authority (nobody can access funds in null account)
#define GRAPHENE_NULL_ACCOUNT_UID (graphene::chain::calc_account_uid(4))
/// Represents the canonical account with WILDCARD authority (anybody can access funds in temp account)
#define GRAPHENE_TEMP_ACCOUNT (graphene::chain::account_id_type(5))
/// Represents the canonical account with WILDCARD authority (anybody can access funds in temp account)
#define GRAPHENE_TEMP_ACCOUNT_UID (graphene::chain::calc_account_uid(5))
/// Sentinel value used in the scheduler.
#define GRAPHENE_NULL_WITNESS (graphene::chain::witness_id_type(0))
///@}

#define GRAPHENE_FBA_STEALTH_DESIGNATED_ASSET (asset_id_type(743))

#define GRAPHENE_MAX_NESTED_OBJECTS (200)

/**
 * Platform configuration
 */
///@{
/// Minimum platform deposit
#define GRAPHENE_DEFAULT_PLATFORM_MIN_PLEDGE (GRAPHENE_BLOCKCHAIN_PRECISION * int64_t(10000))
/// The platform calculates the average deposit duration. The initial value of 1 week
#define GRAPHENE_DEFAULT_PLATFORM_MAX_PLEDGE_SECONDS (60*60*24*7)
/// Platform average deposit update interval. The initial value is 1 hour, that is, the average deposit is automatically updated once an hour, and if the deposit amount changes, the average deposit is updated in real time.
#define GRAPHENE_DEFAULT_PLATFORM_AVG_PLEDGE_UPDATE_INTERVAL 1200 // blocks, one hour if 3 seconds per block
/// Platform deposit refund time (block)
#define GRAPHENE_DEFAULT_PLATFORM_PLEDGE_RELEASE_DELAY 28800 // blocks, one day if 3 seconds per block
/// The maximum number of voting platforms per account
#define GRAPHENE_DEFAULT_PLATFORM_MAX_VOTE_PER_ACCOUNT (uint16_t(10))

#define GRAPHENE_DEFAULT_CONTENT_AWARD_INTERVAL (uint32_t(60*60*24*7))//7 days for award content

#define GRAPHENE_DEFAULT_PLATFORM_AWARD_INTERVAL (uint32_t(60*60*24*30))//1 month for award platform

#define GRAPHENE_DEFAULT_MAX_CSAF_PER_APPROVAL	(GRAPHENE_BLOCKCHAIN_PRECISION * uint64_t(1000))

#define GRAPHENE_DEFAULT_APPROVAL_EXPIRATION	(uint64_t(60*60*24*365)) //1 year, reset approval

#define GRAPHENE_DEFAULT_MIN_EFFECTIVE_CSAF										(GRAPHENE_BLOCKCHAIN_PRECISION * int64_t(100))
#define GRAPHENE_DEFAULT_TOTAL_CONTENT_AWARD_AMOUNT						(GRAPHENE_BLOCKCHAIN_PRECISION * int64_t(0))
#define GRAPHENE_DEFAULT_TOTAL_PLATFORM_CONTENT_AWARD_AMOUNT	(GRAPHENE_BLOCKCHAIN_PRECISION * int64_t(0))
#define GRAPHENE_DEFAULT_TOTAL_PLATFORM_VOTED_AWARD_AMOUNT		(GRAPHENE_BLOCKCHAIN_PRECISION * int64_t(0)) 

#define GRAPHENE_DEFAULT_PLATFORM_AWARD_MIN_VOTES (uint64_t(10))
#define GRAPHENE_DEFAULT_PLATFORM_AWARD_REQUESTED_RANK (uint32_t(100))

#define GRAPHENE_DEFAULT_PLATFORM_RECERPTS_RATIO (GRAPHENE_1_PERCENT*uint32_t(30)) //the ratio of platform`s recerpts from post_object 3000 means 30.00%
///@}
