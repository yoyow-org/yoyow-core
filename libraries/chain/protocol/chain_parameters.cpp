#include <graphene/chain/protocol/chain_parameters.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>

#include <fc/io/raw.hpp>

namespace graphene { namespace chain {
   chain_parameters::chain_parameters() {
       current_fees = std::make_shared<fee_schedule>();
   }

   // copy constructor
   chain_parameters::chain_parameters(const chain_parameters& other)
   {
      current_fees = std::make_shared<fee_schedule>(*other.current_fees);
      safe_copy(*this, other);
   }

   // copy assignment
   chain_parameters& chain_parameters::operator=(const chain_parameters& other)
   {
      if (&other != this)
      {
         current_fees = std::make_shared<fee_schedule>(*other.current_fees);
         safe_copy(*this, other);
      }
      return *this;
   }

   // copies the easy stuff
   void chain_parameters::safe_copy(chain_parameters& to, const chain_parameters& from)
   {
        to.block_interval                          =  from.block_interval;
        to.maintenance_interval                    =  from.maintenance_interval;
        to.maintenance_skip_slots                  =  from.maintenance_skip_slots;
        to.committee_proposal_review_period        =  from.committee_proposal_review_period;
        to.maximum_transaction_size                =  from.maximum_transaction_size;
        to.maximum_block_size                      =  from.maximum_block_size;
        to.maximum_time_until_expiration           =  from.maximum_time_until_expiration;
        to.maximum_proposal_lifetime               =  from.maximum_proposal_lifetime;
        to.maximum_asset_whitelist_authorities     =  from.maximum_asset_whitelist_authorities;
        to.maximum_asset_feed_publishers           =  from.maximum_asset_feed_publishers;
        to.maximum_witness_count                   =  from.maximum_witness_count;
        to.maximum_committee_count                 =  from.maximum_committee_count;
        to.maximum_authority_membership            =  from.maximum_authority_membership;
        to.reserve_percent_of_fee                  =  from.reserve_percent_of_fee;
        to.network_percent_of_fee                  =  from.network_percent_of_fee;
        to.lifetime_referrer_percent_of_fee        =  from.lifetime_referrer_percent_of_fee;
        to.cashback_vesting_period_seconds         =  from.cashback_vesting_period_seconds;
        to.cashback_vesting_threshold              =  from.cashback_vesting_threshold;
        to.count_non_member_votes                  =  from.count_non_member_votes;
        to.allow_non_member_whitelists             =  from.allow_non_member_whitelists;
        to.witness_pay_per_block                   =  from.witness_pay_per_block;
        to.witness_pay_vesting_seconds             =  from.witness_pay_vesting_seconds;
        to.worker_budget_per_day                   =  from.worker_budget_per_day;
        to.max_predicate_opcode                    =  from.max_predicate_opcode;
        to.fee_liquidation_threshold               =  from.fee_liquidation_threshold;
        to.accounts_per_fee_scale                  =  from.accounts_per_fee_scale;
        to.account_fee_scale_bitshifts             =  from.account_fee_scale_bitshifts;
        to.max_authority_depth                     =  from.max_authority_depth;
        to.csaf_rate                               =  from.csaf_rate;
        to.max_csaf_per_account                    =  from.max_csaf_per_account;
        to.csaf_accumulate_window                  =  from.csaf_accumulate_window;
        to.min_witness_pledge                      =  from.min_witness_pledge;
        to.max_witness_pledge_seconds              =  from.max_witness_pledge_seconds;
        to.witness_avg_pledge_update_interval      =  from.witness_avg_pledge_update_interval;
        to.witness_pledge_release_delay            =  from.witness_pledge_release_delay;
        to.min_governance_voting_balance           =  from.min_governance_voting_balance;
        to.max_governance_voting_proxy_level       =  from.max_governance_voting_proxy_level;
        to.governance_voting_expiration_blocks     =  from.governance_voting_expiration_blocks;
        to.governance_votes_update_interval        =  from.governance_votes_update_interval;
        to.max_governance_votes_seconds            =  from.max_governance_votes_seconds;
        to.max_witnesses_voted_per_account         =  from.max_witnesses_voted_per_account;
        to.max_witness_inactive_blocks             =  from.max_witness_inactive_blocks;
        to.by_vote_top_witness_pay_per_block       =  from.by_vote_top_witness_pay_per_block;
        to.by_vote_rest_witness_pay_per_block      =  from.by_vote_rest_witness_pay_per_block;
        to.by_pledge_witness_pay_per_block         =  from.by_pledge_witness_pay_per_block;
        to.by_vote_top_witness_count               =  from.by_vote_top_witness_count;
        to.by_vote_rest_witness_count              =  from.by_vote_rest_witness_count;
        to.by_pledge_witness_count                 =  from.by_pledge_witness_count;
        to.budget_adjust_interval                  =  from.budget_adjust_interval;
        to.budget_adjust_target                    =  from.budget_adjust_target;
        to.committee_size                          =  from.committee_size;
        to.committee_update_interval               =  from.committee_update_interval;
        to.min_committee_member_pledge             =  from.min_committee_member_pledge;
        to.committee_member_pledge_release_delay   =  from.committee_member_pledge_release_delay;
        to.max_committee_members_voted_per_account =  from.max_committee_members_voted_per_account;
        to.witness_report_prosecution_period       =  from.witness_report_prosecution_period;
        to.witness_report_allow_pre_last_block     =  from.witness_report_allow_pre_last_block;
        to.witness_report_pledge_deduction_amount  =  from.witness_report_pledge_deduction_amount;
        to.platform_min_pledge                     =  from.platform_min_pledge;
        to.platform_max_pledge_seconds             =  from.platform_max_pledge_seconds;
        to.platform_avg_pledge_update_interval     =  from.platform_avg_pledge_update_interval;
        to.platform_pledge_release_delay           =  from.platform_pledge_release_delay;
        to.platform_max_vote_per_account           =  from.platform_max_vote_per_account;
        to.extension_parameters                    =  from.extension_parameters;
   }

   // move constructor
   chain_parameters::chain_parameters(chain_parameters&& other)
   {
      current_fees = std::move(other.current_fees);
      safe_copy(*this, other);
   }

   // move assignment
   chain_parameters& chain_parameters::operator=(chain_parameters&& other)
   {
      if (&other != this)
      {
         current_fees = std::move(other.current_fees);
         safe_copy(*this, other);
      }
      return *this;
   }

    void chain_parameters::validate()const
   {
      current_fees->validate();
      FC_ASSERT( reserve_percent_of_fee <= GRAPHENE_100_PERCENT );
      FC_ASSERT( network_percent_of_fee <= GRAPHENE_100_PERCENT );
      FC_ASSERT( lifetime_referrer_percent_of_fee <= GRAPHENE_100_PERCENT );
      FC_ASSERT( network_percent_of_fee + lifetime_referrer_percent_of_fee <= GRAPHENE_100_PERCENT );

      FC_ASSERT( block_interval >= GRAPHENE_MIN_BLOCK_INTERVAL );
      FC_ASSERT( block_interval <= GRAPHENE_MAX_BLOCK_INTERVAL );
      FC_ASSERT( block_interval > 0 );
      FC_ASSERT( maintenance_interval > block_interval,
                 "Maintenance interval must be longer than block interval" );
      FC_ASSERT( maintenance_interval % block_interval == 0,
                 "Maintenance interval must be a multiple of block interval" );
      FC_ASSERT( maximum_transaction_size >= GRAPHENE_MIN_TRANSACTION_SIZE_LIMIT,
                 "Transaction size limit is too low" );
      FC_ASSERT( maximum_block_size >= GRAPHENE_MIN_BLOCK_SIZE_LIMIT,
                 "Block size limit is too low" );
      FC_ASSERT( maximum_time_until_expiration > block_interval,
                 "Maximum transaction expiration time must be greater than a block interval" );
      FC_ASSERT( maximum_proposal_lifetime - committee_proposal_review_period > block_interval,
                 "Committee proposal review period must be less than the maximum proposal lifetime" );
   }
}}
