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
#include <graphene/chain/database.hpp>
#include <graphene/chain/proposal_evaluator.hpp>
#include <graphene/chain/proposal_object.hpp>
#include <graphene/chain/hardfork.hpp>

#include <fc/smart_ref_impl.hpp>

namespace graphene { namespace chain {

void_result proposal_create_evaluator::do_evaluate(const proposal_create_operation& o)
{ try {
   const database& d = db();
   FC_ASSERT( d.head_block_time() >= HARDFORK_0_4_TIME, "Can only be proposal_create after HARDFORK_0_4_TIME" );
   const auto& global_parameters = d.get_global_properties().parameters;

   FC_ASSERT( o.expiration_time > d.head_block_time(), "Proposal has already expired on creation." );
   FC_ASSERT( o.expiration_time <= d.head_block_time() + global_parameters.maximum_proposal_lifetime,
              "Proposal expiration time is too far in the future.");
   FC_ASSERT( !o.review_period_seconds || fc::seconds(*o.review_period_seconds) < (o.expiration_time - d.head_block_time()),
              "Proposal review period must be less than its overall lifetime." );

   for( const op_wrapper& op : o.proposed_ops )
      _proposed_trx.operations.push_back(op.op);
   _proposed_trx.validate();

   vector<authority> other;
   flat_set<account_uid_type> tmp_active;
   flat_set<account_uid_type> tmp_secondary;
   for( auto& op : _proposed_trx.operations )
      operation_get_required_uid_authorities(op,
                                             required_owner,
                                             tmp_active,
                                             tmp_secondary,
                                             other,
                                             d.get_dynamic_global_properties().enabled_hardfork_04);

   std::set_difference(tmp_active.begin(), tmp_active.end(),
                       required_owner.begin(), required_owner.end(),
                       std::inserter(required_active, required_active.begin()));

   std::set_difference(tmp_secondary.begin(), tmp_secondary.end(),
                       required_active.begin(), required_active.end(),
                       std::inserter(required_secondary, required_secondary.begin()));

   auto get_acc_by_uid = [&]( account_uid_type uid ){ return &(d.get_account_by_uid(uid)); };
   flat_set<account_uid_type> tmp;
   std::copy(required_owner.begin(), required_owner.end(), std::inserter(tmp, tmp.end()));
   for( auto uid : tmp )
      get_authority_uid( uid,
                         get_acc_by_uid,
                         required_owner,
                         required_active,
                         required_secondary
                       );
   tmp.clear();

   std::copy(required_active.begin(), required_active.end(), std::inserter(tmp, tmp.end()));
   for( auto uid : tmp )
      get_authority_uid( uid,
                         get_acc_by_uid,
                         required_owner,
                         required_active,
                         required_secondary
                       );
   tmp.clear();

   std::copy(required_secondary.begin(), required_secondary.end(), std::inserter(tmp, tmp.end()));
   for( auto uid : tmp )
      get_authority_uid( uid,
                         get_acc_by_uid,
                         required_owner,
                         required_active,
                         required_secondary
                       );
   tmp.clear();

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

object_id_type proposal_create_evaluator::do_apply(const proposal_create_operation& o)
{ try {
   database& d = db();

   const proposal_object& proposal = d.create<proposal_object>([&](proposal_object& proposal) {
      _proposed_trx.expiration = o.expiration_time;
      proposal.proposed_transaction = _proposed_trx;
      proposal.expiration_time = o.expiration_time;
      if( o.review_period_seconds )
         proposal.review_period_time = o.expiration_time - *o.review_period_seconds;

      std::copy(required_owner.begin(), required_owner.end(), std::inserter(proposal.required_owner_approvals, proposal.required_owner_approvals.end()));
      std::copy(required_active.begin(), required_active.end(), std::inserter(proposal.required_active_approvals, proposal.required_active_approvals.end()));
      std::copy(required_secondary.begin(), required_secondary.end(), std::inserter(proposal.required_secondary_approvals, proposal.required_secondary_approvals.end()));
   });

   return proposal.id;
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result proposal_update_evaluator::do_evaluate(const proposal_update_operation& o)
{ try {
   database& d = db();
   FC_ASSERT( d.head_block_time() >= HARDFORK_0_4_TIME, "Can only be proposal_update after HARDFORK_0_4_TIME" );

   _proposal = &o.proposal(d);

   if( _proposal->review_period_time && d.head_block_time() >= *_proposal->review_period_time )
      FC_ASSERT( o.active_approvals_to_add.empty() &&
                  o.owner_approvals_to_add.empty() && o.secondary_approvals_to_add.empty(),
                 "This proposal is in its review period. No new approvals may be added." );

   for( account_uid_type uid : o.secondary_approvals_to_remove )
   {
      FC_ASSERT( _proposal->available_secondary_approvals.find(uid) != _proposal->available_secondary_approvals.end(),
                 "", ("uid", uid)("available", _proposal->available_secondary_approvals) );
   }
   for( account_uid_type uid : o.active_approvals_to_remove )
   {
      FC_ASSERT( _proposal->available_active_approvals.find(uid) != _proposal->available_active_approvals.end(),
                 "", ("uid", uid)("available", _proposal->available_active_approvals) );
   }
   for( account_uid_type uid : o.owner_approvals_to_remove )
   {
      FC_ASSERT( _proposal->available_owner_approvals.find(uid) != _proposal->available_owner_approvals.end(),
                 "", ("uid", uid)("available", _proposal->available_owner_approvals) );
   }

   /*  All authority checks happen outside of evaluators
   if( (d.get_node_properties().skip_flags & database::skip_authority_check) == 0 )
   {
      for( const auto& id : o.key_approvals_to_add )
      {
         FC_ASSERT( trx_state->signed_by(id) );
      }
      for( const auto& id : o.key_approvals_to_remove )
      {
         FC_ASSERT( trx_state->signed_by(id) );
      }
   }
   */

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result proposal_update_evaluator::do_apply(const proposal_update_operation& o)
{ try {
   database& d = db();

   // Potential optimization: if _executed_proposal is true, we can skip the modify step and make push_proposal skip
   // signature checks. This isn't done now because I just wrote all the proposals code, and I'm not yet 100% sure the
   // required approvals are sufficient to authorize the transaction.
   d.modify(*_proposal, [&o, &d](proposal_object& p) {
      p.available_secondary_approvals.insert(o.secondary_approvals_to_add.begin(), o.secondary_approvals_to_add.end());
      p.available_active_approvals.insert(o.active_approvals_to_add.begin(), o.active_approvals_to_add.end());
      p.available_owner_approvals.insert(o.owner_approvals_to_add.begin(), o.owner_approvals_to_add.end());
      for( account_uid_type uid : o.secondary_approvals_to_remove )
         p.available_secondary_approvals.erase(uid);
      for( account_uid_type uid : o.active_approvals_to_remove )
         p.available_active_approvals.erase(uid);
      for( account_uid_type uid : o.owner_approvals_to_remove )
         p.available_owner_approvals.erase(uid);
      for( const auto& id : o.key_approvals_to_add )
         p.available_key_approvals.insert(id);
      for( const auto& id : o.key_approvals_to_remove )
         p.available_key_approvals.erase(id);
   });

   // If the proposal has a review period, don't bother attempting to authorize/execute it.
   // Proposals with a review period may never be executed except at their expiration.
   if( _proposal->review_period_time )
      return void_result();

   auto result = _proposal->is_authorized_to_execute(d);
   if (result.first)
   {
      // All required approvals are satisfied. Execute!
      //ilog( "apply proposed ${id}", ("id", _proposal->id ) );
      _executed_proposal = true;
      try {
         _processed_transaction = d.push_proposal(*_proposal, result.second);
      } catch(fc::exception& e) {
         wlog("Proposed transaction ${id} failed to apply once approved with exception:\n----\n${reason}\n----\nWill try again when it expires.",
              ("id", o.proposal)("reason", e.to_detail_string()));
         _proposal_failed = true;
      }
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result proposal_delete_evaluator::do_evaluate(const proposal_delete_operation& o)
{ try {
   database& d = db();
   FC_ASSERT( d.head_block_time() >= HARDFORK_0_4_TIME, "Can only be proposal_delete after HARDFORK_0_4_TIME" );

   _proposal = &o.proposal(d);

   bool inowner = _proposal->required_owner_approvals.find( o.fee_paying_account ) != _proposal->required_owner_approvals.end();
   bool inactive = _proposal->required_active_approvals.find( o.fee_paying_account ) != _proposal->required_active_approvals.end();
   bool insecondary = _proposal->required_secondary_approvals.find( o.fee_paying_account ) != _proposal->required_secondary_approvals.end();

   FC_ASSERT( ( inowner || inactive || insecondary ),
              "Provided authority is not authoritative for this proposal.",
              ("provided", o.fee_paying_account));

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result proposal_delete_evaluator::do_apply(const proposal_delete_operation& o)
{ try {
   db().remove(*_proposal);

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

} } // graphene::chain
