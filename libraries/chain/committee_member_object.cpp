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
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <fc/smart_ref_impl.hpp>

namespace graphene { namespace chain {

static fc::smart_ref<fee_schedule> tmp;

void committee_proposal_object::update_approve_threshold()
{
   approve_threshold = get_approve_threshold();
}

struct fee_parameter_get_approve_threshold_visitor
{
   typedef uint16_t result_type;

   result_type operator()( const committee_proposal_create_operation::fee_parameters_type& p )const
   {
      return GRAPHENE_CPPT_FEE_COMMITTEE_MEMBER_CREATE_OP;
   }

   template<typename T>
   result_type operator()( const T& p )const
   {
      return GRAPHENE_CPPT_FEE_DEFAULT;
   }
};

uint16_t committee_proposal_object::get_approve_threshold()const
{
   uint16_t threshold = 0;
   for( const auto& item : items )
   {
      if( item.which() == committee_proposal_item_type::tag< committee_update_account_priviledge_item_type >::value )
      {
         const auto& account_item = item.get< committee_update_account_priviledge_item_type >();
         const auto& pv = account_item.new_priviledges.value;
         if( pv.can_vote.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_ACCOUNT_CAN_VOTE );
         if( pv.is_admin.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_ACCOUNT_IS_ADMIN );
         if( pv.is_registrar.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_ACCOUNT_IS_REGISTRAR );
         if( pv.takeover_registrar.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_ACCOUNT_TAKEOVER_REGISTRAR );
      }
      else if( item.which() == committee_proposal_item_type::tag< committee_update_fee_schedule_item_type >::value )
      {
         const auto& fee_item = item.get< committee_update_fee_schedule_item_type >();
         const fee_parameter_get_approve_threshold_visitor visitor;
         for( const auto& f : fee_item->parameters )
            threshold = std::max( threshold, f.visit( visitor ) );
      }
      else if( item.which() == committee_proposal_item_type::tag< committee_update_global_parameter_item_type >::value )
      {
         const auto& param_item = item.get< committee_update_global_parameter_item_type >();
         const auto& pv = param_item.value;
         if( pv.maximum_transaction_size.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_MAX_TRX_SIZE                          );
         if( pv.maximum_block_size.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_MAX_BLOCK_SIZE                        );
         if( pv.maximum_time_until_expiration.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_MAX_EXPIRATION_TIME                   );
         if( pv.maximum_authority_membership.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_MAX_AUTHORITY_MEMBERSHIP              );
         if( pv.max_authority_depth.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_MAX_AUTHORITY_DEPTH                   );
         if( pv.csaf_rate.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_CSAF_RATE                             );
         if( pv.max_csaf_per_account.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_MAX_CSAF_PER_ACCOUNT                  );
         if( pv.csaf_accumulate_window.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_CSAF_ACCUMULATE_WINDOW                );
         if( pv.min_witness_pledge.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_MIN_WITNESS_PLEDGE                    );
         if( pv.max_witness_pledge_seconds.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_MAX_WITNESS_PLEDGE_SECONDS            );
         if( pv.witness_avg_pledge_update_interval.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_AVG_WITNESS_PLEDGE_UPDATE_INTERVAL    );
         if( pv.witness_pledge_release_delay.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_WITNESS_PLEDGE_RELEASE_DELAY          );
         if( pv.min_governance_voting_balance.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_MIN_GOVERNANCE_VOTING_BALANCE         );
         if( pv.governance_voting_expiration_blocks.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_GOVERNANCE_VOTING_EXPIRATION_BLOCKS   );
         if( pv.governance_votes_update_interval.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_GOVERNANCE_VOTES_UPDATE_INTERVAL      );
         if( pv.max_governance_votes_seconds.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_MAX_GOVERNANCE_VOTES_SECONDS          );
         if( pv.max_witnesses_voted_per_account.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_MAX_WITNESSES_VOTED_PER_ACCOUNT       );
         if( pv.max_witness_inactive_blocks.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_MAX_WITNESS_INACTIVE_BLOCKS           );
         if( pv.by_vote_top_witness_pay_per_block.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_BY_VOTE_TOP_WITNESS_PAY_PER_BLOCK     );
         if( pv.by_vote_rest_witness_pay_per_block.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_BY_VOTE_REST_WITNESS_PAY_PER_BLOCK    );
         if( pv.by_pledge_witness_pay_per_block.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_BY_PLEDGE_WITNESS_PAY_PER_BLOCK       );
         if( pv.by_vote_top_witness_count.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_BY_VOTE_TOP_WITNESS_COUNT             );
         if( pv.by_vote_rest_witness_count.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_BY_VOTE_REST_WITNESS_COUNT            );
         if( pv.by_pledge_witness_count.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_BY_PLEDGE_WITNESS_COUNT               );
         if( pv.budget_adjust_interval.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_BUDGET_ADJUST_INTERVAL                );
         if( pv.budget_adjust_target.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_BUDGET_ADJUST_TARGET                  );
         if( pv.min_committee_member_pledge.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_MIN_COMMITTEE_MEMBER_PLEDGE           );
         if( pv.committee_member_pledge_release_delay.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_COMMITTEE_MEMBER_PLEDGE_RELEASE_DELAY );
         if( pv.witness_report_prosecution_period.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_WITNESS_REPORT_PROSECUTION_PERIOD     );
         if( pv.witness_report_allow_pre_last_block.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_WITNESS_REPORT_ALLOW_PRE_LAST_BLOCK   );
         if( pv.witness_report_pledge_deduction_amount.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_WITNESS_REPORT_PLEDGE_DEDUCTION_AMOUNT );
         if( pv.platform_min_pledge.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_PLATFORM_MIN_PLEDGE                    );
         if( pv.platform_pledge_release_delay.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_PLATFORM_PLEDGE_RELEASE_DELAY          );
         if( pv.platform_max_vote_per_account.valid() )
            threshold = std::max( threshold, GRAPHENE_CPPT_PARAM_PLATFORM_MAX_VOTE_PER_ACCOUNT          );

      }
   }
   return threshold;
}

} } // graphene::chain
