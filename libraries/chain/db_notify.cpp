#include <fc/container/flat.hpp>

#include <graphene/chain/protocol/authority.hpp>
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/protocol/transaction.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/impacted.hpp>

using namespace fc;

namespace graphene { namespace chain {

// TODO:  Review all of these, especially no-ops
struct get_impacted_account_uid_visitor
{
   flat_set<account_uid_type>& _impacted;
   get_impacted_account_uid_visitor( flat_set<account_uid_type>& impact ):_impacted(impact) {}
   typedef void result_type;

   // NOTE: don't use a default template operator, since it may cause unintended behavior

   void operator()( const account_create_operation& op )
   {
      _impacted.insert( op.uid );
      _impacted.insert( op.reg_info.registrar ); // fee payer
      _impacted.insert( op.reg_info.referrer );
      add_authority_account_uids( _impacted, op.owner );
      add_authority_account_uids( _impacted, op.active );
      add_authority_account_uids( _impacted, op.secondary );
   }

   void operator()( const transfer_operation& op )
   {
      _impacted.insert( op.from ); // fee payer
      _impacted.insert( op.to );
   }

   void operator()( const post_operation& op )
   {
      _impacted.insert( op.poster ); // fee payer

      _impacted.insert( op.platform );
      if( op.origin_platform.valid() )
          _impacted.insert( *(op.origin_platform) );
      if( op.origin_poster.valid() )
          _impacted.insert( *(op.origin_poster) );
      if (op.extensions.valid()){
          if (op.extensions->value.receiptors.valid()){
              auto receiptors = *(op.extensions->value.receiptors);
              for (auto iter : receiptors)
                  _impacted.insert(iter.first);
          }
      }
   }

   void operator()( const post_update_operation& op )
   {
      _impacted.insert( op.poster ); // fee payer
      _impacted.insert( op.platform );
      if (op.extensions.valid()){
          auto ext = op.extensions->value;
          if (ext.receiptor.valid())
              _impacted.insert(*(ext.receiptor));
      }
   }

   void operator()( const account_manage_operation& op )
   {
      _impacted.insert( op.executor ); // fee payer
      _impacted.insert( op.account );
   }

   void operator()( const csaf_collect_operation& op )
   {
      _impacted.insert( op.from ); // fee payer
      _impacted.insert( op.to );
   }

   void operator()( const csaf_lease_operation& op )
   {
      _impacted.insert( op.from ); // fee payer
      _impacted.insert( op.to );
   }

   void operator()( const account_update_key_operation& op )
   {
      _impacted.insert( op.fee_paying_account ); // fee payer
      _impacted.insert( op.uid );
   }

   void operator()( const account_update_auth_operation& op )
   {
      _impacted.insert( op.uid ); // fee payer
      if( op.owner.valid() )
         add_authority_account_uids( _impacted, *op.owner );
      if( op.active.valid() )
         add_authority_account_uids( _impacted, *op.active );
      if( op.secondary.valid() )
         add_authority_account_uids( _impacted, *op.secondary );
   }

   void operator()( const account_auth_platform_operation& op )
   {
      _impacted.insert( op.uid ); // fee payer
      _impacted.insert( op.platform );
   }

   void operator()( const account_cancel_auth_platform_operation& op )
   {
      _impacted.insert( op.uid ); // fee payer
      _impacted.insert( op.platform );
   }

   void operator()( const account_update_proxy_operation& op )
   {
      _impacted.insert( op.voter ); // fee payer
      _impacted.insert( op.proxy );
   }

   void operator()( const account_enable_allowed_assets_operation& op )
   {
      _impacted.insert( op.account ); // fee payer
   }

   void operator()( const account_update_allowed_assets_operation& op )
   {
      _impacted.insert( op.account ); // fee payer
   }

   void operator()( const witness_create_operation& op )
   {
      _impacted.insert( op.account ); // fee payer
   }

   void operator()( const witness_update_operation& op )
   {
      _impacted.insert( op.account ); // fee payer
   }

   void operator()( const witness_vote_update_operation& op )
   {
      _impacted.insert( op.voter ); // fee payer
      for( auto uid : op.witnesses_to_add )
         _impacted.insert( uid );
      for( auto uid : op.witnesses_to_remove )
         _impacted.insert( uid );
   }

   void operator()( const witness_collect_pay_operation& op )
   {
      _impacted.insert( op.account ); // fee payer
   }

   void operator()( const witness_report_operation& op )
   {
      _impacted.insert( op.reporter ); // fee payer
      _impacted.insert( op.first_block.witness );
   }

   void operator()( const platform_create_operation& op )
   {
      _impacted.insert( op.account ); // fee payer
   }

   void operator()( const platform_update_operation& op )
   {
      _impacted.insert( op.account ); // fee payer
   }

   void operator()( const platform_vote_update_operation& op )
   {
      _impacted.insert( op.voter ); // fee payer
      for( auto uid : op.platform_to_add )
         _impacted.insert( uid );
      for( auto uid : op.platform_to_remove )
         _impacted.insert( uid );
   }

   void operator()( const committee_member_create_operation& op )
   {
      _impacted.insert( op.account ); // fee payer
   }
   void operator()( const committee_member_update_operation& op )
   {
      _impacted.insert( op.account ); // fee payer
   }
   void operator()( const committee_member_vote_update_operation& op )
   {
      _impacted.insert( op.voter ); // fee payer
      for( auto uid : op.committee_members_to_add )
         _impacted.insert( uid );
      for( auto uid : op.committee_members_to_remove )
         _impacted.insert( uid );
   }
   void operator()( const committee_proposal_create_operation& op )
   {
      _impacted.insert( op.proposer ); // fee payer
      for( const auto& item : op.items )
      {
         if( item.which() == committee_proposal_item_type::tag< committee_update_account_priviledge_item_type >::value )
         {
            const auto& account_item = item.get< committee_update_account_priviledge_item_type >();
            _impacted.insert( account_item.account );
            if( account_item.new_priviledges.value.takeover_registrar.valid() )
               _impacted.insert( *account_item.new_priviledges.value.takeover_registrar );
         }
      }
   }
   void operator()( const committee_proposal_update_operation& op )
   {
      _impacted.insert( op.account ); // fee payer
   }

   void operator()( const asset_create_operation& op )
   {
      _impacted.insert( op.issuer ); // fee payer
      for( auto uid : op.common_options.whitelist_authorities )
         _impacted.insert( uid );
      for( auto uid : op.common_options.blacklist_authorities )
         _impacted.insert( uid );
   }
   void operator()( const asset_update_operation& op )
   {
      _impacted.insert( op.issuer ); // fee payer
      for( auto uid : op.new_options.whitelist_authorities )
         _impacted.insert( uid );
      for( auto uid : op.new_options.blacklist_authorities )
         _impacted.insert( uid );
   }

   void operator()( const asset_issue_operation& op )
   {
      _impacted.insert( op.issuer ); // fee payer
      _impacted.insert( op.issue_to_account );
   }

   void operator()( const asset_reserve_operation& op )
   {
      _impacted.insert( op.payer ); // fee payer
   }

   void operator()( const asset_claim_fees_operation& op )
   {
      _impacted.insert( op.issuer ); // fee payer
   }

   void operator()( const override_transfer_operation& op )
   {
      _impacted.insert( op.to );
      _impacted.insert( op.from );
      _impacted.insert( op.issuer ); // fee payer
   }

   void operator()( const proposal_create_operation& op )
   {
      _impacted.insert( op.fee_paying_account ); // fee payer
      vector<authority> other;
      for( const auto& proposed_op : op.proposed_ops )
         operation_get_required_uid_authorities( proposed_op.op, _impacted, _impacted, _impacted, other,true );
      for( auto& o : other )
         add_authority_account_uids( _impacted, o );
   }

   // TODO review
   void operator()( const account_whitelist_operation& op )
   {
      _impacted.insert( op.account_to_list );
      _impacted.insert( op.fee_payer_uid() ); // fee payer
   }

   // TODO review
   void operator()( const proposal_update_operation& op )
   {
      _impacted.insert( op.fee_payer_uid() ); // fee payer
   }

   // TODO review
   void operator()( const proposal_delete_operation& op )
   {
      _impacted.insert( op.fee_payer_uid() ); // fee payer
   }

   // TODO review
   void operator()(const score_create_operation& op)
   {
	   _impacted.insert(op.fee_payer_uid()); // fee payer
       _impacted.insert(op.platform);
       _impacted.insert(op.poster);
   }

   // TODO review
   void operator()(const reward_operation& op)
   {
	   _impacted.insert(op.fee_payer_uid()); // fee payer
       _impacted.insert(op.platform);
       _impacted.insert(op.poster);
   }

   // TODO review
   void operator()(const reward_proxy_operation& op)
   {
       _impacted.insert(op.fee_payer_uid()); // fee payer
       _impacted.insert(op.platform);
       _impacted.insert(op.poster);
   }

   // TODO review
   void operator()(const buyout_operation& op)
   {
	   _impacted.insert(op.fee_payer_uid()); // fee payer
       _impacted.insert(op.platform);
       _impacted.insert(op.poster);
       _impacted.insert(op.receiptor_account_uid);
   }

   void operator()(const license_create_operation& op)
   {
     _impacted.insert(op.fee_payer_uid()); // fee payer
   }

   void operator()(const advertising_create_operation& op)
   {
       _impacted.insert(op.fee_payer_uid()); // fee payer
   }

   void operator()(const advertising_update_operation& op)
   {
       _impacted.insert(op.fee_payer_uid()); // fee payer
   }

   void operator()(const advertising_buy_operation& op)
   {
       _impacted.insert(op.fee_payer_uid()); // fee payer
       _impacted.insert(op.platform);
   }

   void operator()(const advertising_confirm_operation& op)
   {
       _impacted.insert(op.fee_payer_uid()); // fee payer
   }

   void operator()(const advertising_ransom_operation& op)
   {
       _impacted.insert(op.fee_payer_uid()); // fee payer
       _impacted.insert(op.platform);
   }

   void operator()(const custom_vote_create_operation& op)
   {
       _impacted.insert(op.fee_payer_uid()); // fee payer
   }

   void operator()(const custom_vote_cast_operation& op)
   {
       _impacted.insert(op.fee_payer_uid()); // fee payer
   }

   void operator()(const balance_lock_update_operation& op)
   {
      _impacted.insert(op.fee_payer_uid()); // fee payer
   }

   void operator()(const pledge_mining_update_operation& op)
   {
      _impacted.insert(op.fee_payer_uid()); // fee payer
      _impacted.insert(op.witness);
   }

   void operator()(const pledge_bonus_collect_operation& op)
   {
      _impacted.insert(op.fee_payer_uid()); // fee payer
   }

   void operator()(const limit_order_create_operation& op)
   {
       _impacted.insert(op.fee_payer_uid()); // seller
   }
   void operator()(const limit_order_cancel_operation& op)
   {
       _impacted.insert(op.fee_payer_uid()); // fee_paying_account
   }
   void operator()(const fill_order_operation& op)
   {
       _impacted.insert(op.fee_payer_uid()); // account_id
   }
};

void operation_get_impacted_account_uids( const operation& op, flat_set<account_uid_type>& result )
{
   get_impacted_account_uid_visitor vtor = get_impacted_account_uid_visitor( result );
   op.visit( vtor );
}

void transaction_get_impacted_account_uids( const transaction& tx, flat_set<account_uid_type>& result )
{
   for( const auto& op : tx.operations )
      operation_get_impacted_account_uids( op, result );
}

void get_relevant_accounts( const object* obj, flat_set<account_uid_type>& accounts )
{
   if( obj->id.space() == protocol_ids )
   {
      switch( (object_type)obj->id.type() )
      {
        case null_object_type:
        case base_object_type:
        case OBJECT_TYPE_COUNT:
           return;
        case account_object_type:{
           accounts.insert( ((account_object*)obj)->uid );
           break;
        } case asset_object_type:{
           const auto& aobj = dynamic_cast<const asset_object*>(obj);
           assert( aobj != nullptr );
           accounts.insert( aobj->issuer );
           break;
        } case platform_object_type:{
           const auto& aobj = dynamic_cast<const platform_object*>(obj);
           assert( aobj != nullptr );
           accounts.insert( aobj->owner );
           break;
        } case post_object_type:{
           const auto& aobj = dynamic_cast<const post_object*>(obj);
           assert( aobj != nullptr );
           accounts.insert( aobj->poster );
           if( aobj->origin_poster.valid() )
              accounts.insert( *(aobj->origin_poster) );
           break;
        } case committee_member_object_type:{
           const auto& aobj = dynamic_cast<const committee_member_object*>(obj);
           assert( aobj != nullptr );
           accounts.insert( aobj->account );
           break;
        } case committee_proposal_object_type:{
           const auto& aobj = dynamic_cast<const committee_proposal_object*>(obj);
           assert( aobj != nullptr );
           accounts.insert( aobj->proposer );
           break;
        } case witness_object_type:{
           const auto& aobj = dynamic_cast<const witness_object*>(obj);
           assert( aobj != nullptr );
           accounts.insert( aobj->account );
           break;
        } case proposal_object_type:{
           const auto& aobj = dynamic_cast<const proposal_object*>(obj);
           assert( aobj != nullptr );
           transaction_get_impacted_account_uids( aobj->proposed_transaction, accounts );
           break;
        } case operation_history_object_type:{
           const auto& aobj = dynamic_cast<const operation_history_object*>(obj);
           assert( aobj != nullptr );
           operation_get_impacted_account_uids( aobj->op, accounts );
           break;
        } case active_post_object_type:{
           const auto& aobj = dynamic_cast<const active_post_object*>(obj);
           assert( aobj != nullptr );
           accounts.insert( aobj->platform );
           accounts.insert( aobj->poster );
           break;
        }case limit_order_object_type:{
            const auto& aobj = dynamic_cast<const limit_order_object*>(obj);
            FC_ASSERT(aobj != nullptr);
            accounts.insert(aobj->seller);
            break;
        }
      }
   }
   else if( obj->id.space() == implementation_ids )
   {
      switch( (impl_object_type)obj->id.type() )
      {
             case IMPL_OBJECT_TYPE_COUNT:
              break;
             case impl_global_property_object_type:
              break;
             case impl_dynamic_global_property_object_type:
              break;
             case impl_asset_dynamic_data_type:
              break;
             case impl_account_balance_object_type:{
              const auto& aobj = dynamic_cast<const account_balance_object*>(obj);
              assert( aobj != nullptr );
              accounts.insert( aobj->owner );
              break;
           } case impl_account_statistics_object_type:{
              const auto& aobj = dynamic_cast<const account_statistics_object*>(obj);
              assert( aobj != nullptr );
              accounts.insert( aobj->owner );
              break;
            } case impl_voter_object_type:{
               // TODO review
              break;
            } case impl_witness_vote_object_type:{
               // TODO review
              break;
            } case impl_platform_vote_object_type:{
              // TODO review
              break;
			} case impl_score_object_type:{
				// TODO review
				break;
            } case impl_license_object_type:{
                // TODO review
                break;
            } case impl_advertising_object_type:{
                // TODO review
                break;
            } case impl_advertising_order_object_type:{
                // TODO review
                break;
            } case impl_custom_vote_object_type:{
                // TODO review
                break;
            } case impl_cast_custom_vote_object_type:{
                // TODO review
                break;
            } case impl_committee_member_vote_object_type:{
               // TODO review
              break;
            } case impl_registrar_takeover_object_type:{
               // TODO review
              break;
            } case impl_csaf_lease_object_type:{
               const auto& aobj = dynamic_cast<const csaf_lease_object*>(obj);
               assert( aobj != nullptr );
               accounts.insert( aobj->from );
               accounts.insert( aobj->to );
               break;
           } case impl_transaction_object_type:{
              const auto& aobj = dynamic_cast<const transaction_object*>(obj);
              assert( aobj != nullptr );
              transaction_get_impacted_account_uids( aobj->trx, accounts );
              break;
           } case impl_block_summary_object_type:
              break;
             case impl_account_transaction_history_object_type:
              break;
             case impl_chain_property_object_type:
              break;
             case impl_witness_schedule_object_type:
              break;
             case impl_account_auth_platform_object_type:
              break;
      }
   }
}

void database::notify_changed_objects()
{ try {
   if( _undo_db.enabled() )
   {
      const auto& head_undo = _undo_db.head();

      // New
      if( !new_objects.empty() )
      {
        vector<object_id_type> new_ids;  new_ids.reserve(head_undo.new_ids.size());
        flat_set<account_uid_type> new_accounts_impacted;
        for( const auto& item : head_undo.new_ids )
        {
          new_ids.push_back(item);
          auto obj = find_object(item);
          if(obj != nullptr)
            get_relevant_accounts(obj, new_accounts_impacted);
        }

        new_objects(new_ids, new_accounts_impacted);
      }

      // Changed
      if( !changed_objects.empty() )
      {
        vector<object_id_type> changed_ids;  changed_ids.reserve(head_undo.old_values.size());
        flat_set<account_uid_type> changed_accounts_impacted;
        for( const auto& item : head_undo.old_values )
        {
          changed_ids.push_back(item.first);
          get_relevant_accounts(item.second.get(), changed_accounts_impacted);
        }

        changed_objects(changed_ids, changed_accounts_impacted);
      }

      // Removed
      if( !removed_objects.empty() )
      {
        vector<object_id_type> removed_ids; removed_ids.reserve( head_undo.removed.size() );
        vector<const object*> removed; removed.reserve( head_undo.removed.size() );
        flat_set<account_uid_type> removed_accounts_impacted;
        for( const auto& item : head_undo.removed )
        {
          removed_ids.emplace_back( item.first );
          auto obj = item.second.get();
          removed.emplace_back( obj );
          get_relevant_accounts(obj, removed_accounts_impacted);
        }

        removed_objects(removed_ids, removed, removed_accounts_impacted);
      }
   }
} FC_CAPTURE_AND_LOG( (0) ) }

} }
