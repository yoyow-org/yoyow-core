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
#include <graphene/chain/protocol/committee_member.hpp>

namespace graphene { namespace chain {

void committee_member_create_operation::validate()const
{
   validate_op_fee( fee, "committee member creation " );
   validate_account_uid( account, "committee member " );
   validate_non_negative_core_asset( pledge, "pledge" );
   FC_ASSERT( url.size() < GRAPHENE_MAX_URL_LENGTH, "url is too long" );
}

void committee_member_update_operation::validate()const
{
   validate_op_fee( fee, "committee member update " );
   validate_account_uid( account, "committee member " );
   FC_ASSERT( new_pledge.valid() || new_url.valid(), "Should change something" );
   if( new_pledge.valid() )
      validate_non_negative_core_asset( *new_pledge, "new pledge" );
   if( new_url.valid() )
      FC_ASSERT( new_url->size() < GRAPHENE_MAX_URL_LENGTH, "new url is too long" );
}

void committee_member_vote_update_operation::validate() const
{
   validate_op_fee( fee, "committee member vote update " );
   validate_account_uid( voter, "voter " );
   auto itr_add = committee_members_to_add.begin();
   auto itr_remove = committee_members_to_remove.begin();
   while( itr_add != committee_members_to_add.end() && itr_remove != committee_members_to_remove.end() )
   {
      FC_ASSERT( *itr_add != *itr_remove, "Can not add and remove same committee member, uid: ${w}", ("w",*itr_add) );
      if( *itr_add < *itr_remove )
         ++itr_add;
      else
         ++itr_remove;
   }
   for( const auto uid : committee_members_to_add )
      validate_account_uid( uid, "committee member " );
   for( const auto uid : committee_members_to_remove )
      validate_account_uid( uid, "committee member " );
   // can change nothing to only refresh last_vote_update_time
}

share_type committee_proposal_create_operation::calculate_fee( const fee_parameters_type& k )const
{
   share_type core_fee_required = k.basic_fee;

   auto total_size = items.size();
   core_fee_required += ( share_type( k.price_per_item ) * total_size );

   return core_fee_required;
}

void committee_update_account_priviledge_item_type::validate()const
{
   validate_account_uid( account, "target " );
   bool to_update_something =  new_priviledges.value.can_vote.valid()
                            || new_priviledges.value.is_admin.valid()
                            || new_priviledges.value.is_registrar.valid()
                            || new_priviledges.value.takeover_registrar.valid();
   FC_ASSERT( to_update_something, "Should update something for account ${a}", ("a",account) );

   if( new_priviledges.value.is_registrar.valid() )
   {
      if( *new_priviledges.value.is_registrar == false )
         FC_ASSERT( new_priviledges.value.takeover_registrar.valid(), "Should specify a takeover registrar account" );
      else // the account is updated to be a registrar
         FC_ASSERT( !new_priviledges.value.takeover_registrar.valid(), "Should not specify a takeover registrar account" );
   }

   if( new_priviledges.value.takeover_registrar.valid() )
   {
      FC_ASSERT( account != *new_priviledges.value.takeover_registrar , "Takeover registrar account should not be self" );
      validate_account_uid( *new_priviledges.value.takeover_registrar, "takeover registrar " );
   }
}

void committee_updatable_parameters::validate()const
{
   if( maximum_transaction_size.valid() )
      FC_ASSERT( *maximum_transaction_size >= GRAPHENE_MIN_TRANSACTION_SIZE_LIMIT,
                 "Transaction size limit is too low" );
   if( maximum_block_size.valid() )
      FC_ASSERT( *maximum_block_size >= GRAPHENE_MIN_BLOCK_SIZE_LIMIT,
                 "Block size limit is too low" );
   // this is checked in evaluator
   //if( maximum_time_until_expiration.valid() )
   //   FC_ASSERT( *maximum_time_until_expiration > block_interval,
   //              "Maximum transaction expiration time must be greater than a block interval" );
   if( maximum_authority_membership.valid() )
      FC_ASSERT( *maximum_authority_membership > 0,
                 "Maximum authority membership should be positive" );
   // max_authority_depth: no limitation so far
   if( csaf_rate.valid() )
      FC_ASSERT( *csaf_rate > 0,
                 "CSAF rate should be positive" );
   if( max_csaf_per_account.valid() )
      FC_ASSERT( *max_csaf_per_account >= 0,
                 "Maximum CSAF per account should not be negative" );
   // csaf_accumulate_window: no limitation so far
   // min_witness_pledge: no limitation so far
   if( max_witness_pledge_seconds.valid() )
      FC_ASSERT( *max_witness_pledge_seconds > 0,
                 "Maximum witness pledge accumulation seconds must be positive" );
   if( witness_avg_pledge_update_interval.valid() )
      FC_ASSERT( *witness_avg_pledge_update_interval > 0,
                 "Witness average pledge update interval must be positive" );
   // witness_pledge_release_delay: no limitation so far
   // min_governance_voting_balance: no limitation so far
   // max_governance_voting_proxy_level: unable to update so far
   if( governance_voting_expiration_blocks.valid() )
      FC_ASSERT( *governance_voting_expiration_blocks >= GRAPHENE_MIN_GOVERNANCE_VOTING_EXPIRATION_BLOCKS,
                 "Governance voting expiration blocks should be larger" );
   if( governance_votes_update_interval.valid() )
      FC_ASSERT( *governance_votes_update_interval > 0,
                 "Governance votes update interval must be positive" );
   if( max_governance_votes_seconds.valid() )
      FC_ASSERT( *max_governance_votes_seconds > 0,
                 "Governance votes accumulation seconds must be positive" );
   if( max_witnesses_voted_per_account.valid() )
      FC_ASSERT( *max_witnesses_voted_per_account > 0,
                 "Maximum witnesses voted per account must be positive" );
   // max_witness_inactive_blocks: no limitation so far
   if( by_vote_top_witness_pay_per_block.valid() )
      FC_ASSERT( *by_vote_top_witness_pay_per_block >= 0,
                 "Witness pay per block should not be negative" );
   if( by_vote_rest_witness_pay_per_block.valid() )
      FC_ASSERT( *by_vote_rest_witness_pay_per_block >= 0,
                 "Witness pay per block should not be negative" );
   if( by_pledge_witness_pay_per_block.valid() )
      FC_ASSERT( *by_pledge_witness_pay_per_block >= 0,
                 "Witness pay per block should not be negative" );
   if( by_vote_top_witness_count.valid() )
      FC_ASSERT( *by_vote_top_witness_count > 0,
                 "Top witness count should be positive" );
   // by_vote_rest_witness_count: no limitation so far
   // by_pledge_witness_count: no limitation so far
   if( budget_adjust_interval.valid() )
      FC_ASSERT( *budget_adjust_interval > 0,
                 "Budget adjustment interval should be positive" );
   // budget_adjust_target: no limitation so far
   // committee_size: unable to update so far
   // committee_update_interval: unable to update so far
   // min_committee_member_pledge: no limitation so far
   // committee_member_pledge_release_delay: no limitation so far
   // max_committee_members_voted_per_account: unable to update so far
   // witness_report_prosecution_period: no limitation so far
   // witness_report_allow_pre_last_block: no limitation so far
   if( witness_report_pledge_deduction_amount.valid() )
      FC_ASSERT( *witness_report_pledge_deduction_amount >= 0,
                 "Witness pledge deduction amount should not be negative" );
   // platform_min_pledge: no limitation so far
   // platform_pledge_release_delay: no limitation so far
   if( platform_max_vote_per_account.valid() )
      FC_ASSERT( *platform_max_vote_per_account > 0,
                 "Maximum platforms voted per account must be positive" );
   if( platform_max_pledge_seconds.valid() )
      FC_ASSERT( *platform_max_pledge_seconds > 0,
                 "Maximum platform pledge accumulation seconds must be positive" );
   if( platform_avg_pledge_update_interval.valid() )
      FC_ASSERT( *platform_avg_pledge_update_interval > 0,
                 "Platform average pledge update interval must be positive" );
}

void committee_updatable_content_parameters::validate()const
{
   if (approval_casf_min_weight.valid() && approval_casf_first_rate.valid() && approval_casf_second_rate.valid())
   {
      FC_ASSERT(*approval_casf_min_weight >= 0 && *approval_casf_min_weight <= GRAPHENE_100_PERCENT,
         "approval casf min weight must be be in range 0-100%");
      FC_ASSERT(*approval_casf_first_rate >= 0,
         "approval casf first rate must be positive");
      FC_ASSERT(*approval_casf_second_rate <= GRAPHENE_100_PERCENT,
         "approval casf second rate must be not greater than 100%");
      FC_ASSERT(*approval_casf_second_rate > *approval_casf_first_rate,
         "approval casf second rate must greater than first rate");
   }
   else if (!approval_casf_min_weight.valid() && !approval_casf_first_rate.valid() && !approval_casf_second_rate.valid())
   {
      //do nothing
   }     
   else
      FC_ASSERT(false, "min weight,first rate and second rate should be updated together");

   if (receiptor_award_modulus.valid() && disapprove_award_modulus.valid())
   {
      FC_ASSERT(*receiptor_award_modulus >= 0 && *receiptor_award_modulus <= GRAPHENE_100_PERCENT,
         "receiptor award modulus must be be in range 0-100%");
      FC_ASSERT(*disapprove_award_modulus > GRAPHENE_100_PERCENT,
         "receiptor award modulus must be greater than 100%");
      auto receiptor_remaining_award = GRAPHENE_RECEIPTOR_AWARD_THRESHOLD * (GRAPHENE_100_PERCENT - *receiptor_award_modulus);
      auto disapprove_extra_award = (GRAPHENE_100_PERCENT - GRAPHENE_RECEIPTOR_AWARD_THRESHOLD)* (*disapprove_award_modulus - GRAPHENE_100_PERCENT);
      FC_ASSERT(receiptor_remaining_award >= disapprove_extra_award,
         "extra award for disapprove should less than receiptor remaining award");
   }
   else if (!receiptor_award_modulus.valid() && !disapprove_award_modulus.valid())
   {
      //do nothing
   }
   else
      FC_ASSERT(false, "receiptor award modulus and disapprove award modulus should be updated together");
   
   if (platform_award_basic_rate.valid())
      FC_ASSERT(*platform_award_basic_rate > 0 && *platform_award_basic_rate < GRAPHENE_100_PERCENT,
               "platform award basic rate should be in range 0-100%");
   if (advertising_confirmed_fee_rate.valid())
      FC_ASSERT(*advertising_confirmed_fee_rate > 0 && *advertising_confirmed_fee_rate < GRAPHENE_100_PERCENT, 
               "advertising confirmed fee rate should be in range 0-100%");
   if (min_witness_block_produce_pledge.valid())
      FC_ASSERT(*min_witness_block_produce_pledge > 0, "min witness block produce pledge must be positive");
}

void committee_proposal_create_operation::validate()const
{
   validate_op_fee( fee, "committee proposal creation " );
   validate_account_uid( proposer, "proposer " );
   FC_ASSERT( items.size() > 0, "Should propose something" );
   FC_ASSERT( expiration_block_num >= execution_block_num,
              "Expiration block number should not be earlier than execution block number" );
   uint32_t account_item_count = 0;
   uint32_t fee_item_count = 0;
   uint32_t param_item_count = 0;
   uint32_t award_item_count = 0;
   //flat_map< account_uid_type, committee_update_account_priviledge_item_type::account_priviledge_update_options > account_items;
   for( const auto& item : items )
   {
      if( item.which() == committee_proposal_item_type::tag< committee_update_account_priviledge_item_type >::value )
      {
         account_item_count += 1;
         const auto& account_item = item.get< committee_update_account_priviledge_item_type >();
         account_item.validate();
         //const auto& result = account_items.insert( std::make_pair( account_item.account, account_item.new_priviledges.value ) );
         //FC_ASSERT( result.second, "Duplicate target accounts detected" );
      }
      else if( item.which() == committee_proposal_item_type::tag< committee_update_fee_schedule_item_type >::value )
      {
         fee_item_count += 1;
         FC_ASSERT( fee_item_count <= 1, "No more than one fee schedule update item is allowed" );
         // TODO check if really need to validate
         //const auto& fee_item = item.get< committee_update_fee_schedule_item_type >();
         //fee_item->validate();
      }
      else if( item.which() == committee_proposal_item_type::tag< committee_update_global_parameter_item_type >::value )
      {
         param_item_count += 1;
         FC_ASSERT( param_item_count <= 1, "No more than one global parameter update item is allowed" );
         const auto& param_item = item.get< committee_update_global_parameter_item_type >();
         param_item.value.validate();
      }
      else if (item.which() == committee_proposal_item_type::tag< committee_update_global_content_parameter_item_type >::value)
      {
         award_item_count += 1;
         FC_ASSERT(award_item_count <= 1, "No more than one global parameter award update item is allowed");
         const auto& param_item = item.get< committee_update_global_content_parameter_item_type >();
         param_item.value.validate();
      }
      else
         FC_ASSERT( false, "Bad proposal item type: ${n}", ("n",item.which()) );
   }

}

void committee_proposal_update_operation::validate()const
{
   validate_op_fee( fee, "committee proposal update " );
   validate_account_uid( account, "committee member " );
}

} } // graphene::chain
