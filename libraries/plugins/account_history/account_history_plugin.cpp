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

#include <graphene/account_history/account_history_plugin.hpp>

#include <graphene/chain/impacted.hpp>
#include <graphene/chain/account_evaluator.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/config.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/operation_history_object.hpp>
#include <graphene/chain/transaction_evaluation_state.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

namespace graphene { namespace account_history {

namespace detail
{


class account_history_plugin_impl
{
   public:
      account_history_plugin_impl(account_history_plugin& _plugin)
         : _self( _plugin )
      { }
      virtual ~account_history_plugin_impl();


      /** this method is called as a callback after a block is applied
       * and will process/index all operations that were applied in the block.
       */
      void update_account_histories( const signed_block& b );

      graphene::chain::database& database()
      {
         return _self.database();
      }

      account_history_plugin& _self;
      flat_set<account_uid_type> _tracked_accounts;
      bool _partial_operations = false;
      primary_index< operation_history_index >* _oho_index;
      uint32_t _max_ops_per_account = -1;
   private:
      /** add one history record, then check and remove the earliest history record */
      void add_account_history( const account_uid_type account_uid, const operation_history_id_type op_id, uint16_t op_type );
};

account_history_plugin_impl::~account_history_plugin_impl()
{
   return;
}

void account_history_plugin_impl::update_account_histories( const signed_block& b )
{
   if( b.block_num() == 5881511 ) // skip this block
      return;
   graphene::chain::database& db = database();
   const vector<optional< operation_history_object > >& hist = db.get_applied_operations();
   bool is_first = true;
   auto skip_oho_id = [&is_first,&db,this]() {
      if( is_first && db._undo_db.enabled() ) // this ensures that the current id is rolled back on undo
      {
         db.remove( db.create<operation_history_object>( []( operation_history_object& obj) {} ) );
         is_first = false;
      }
      else
         _oho_index->use_next_id();
   };
   for( const optional< operation_history_object >& o_op : hist )
   {
      optional<operation_history_object> oho;

      auto create_oho = [&]() {
         is_first = false;
         return optional<operation_history_object>( db.create<operation_history_object>( [&]( operation_history_object& h )
         {
            if( o_op.valid() )
            {
               h.op           = o_op->op;
               h.result       = o_op->result;
               h.block_num    = o_op->block_num;
               h.trx_in_block = o_op->trx_in_block;
               h.op_in_trx    = o_op->op_in_trx;
               h.virtual_op   = o_op->virtual_op;
               h.block_timestamp = o_op->block_timestamp;
            }
         } ) );
      };

      if( !o_op.valid() || ( _max_ops_per_account == 0 && _partial_operations ) )
      {
         // Note: the 2nd and 3rd checks above are for better performance, when the db is not clean,
         //       they will break consistency of account_stats.total_ops and removed_ops and most_recent_op
         skip_oho_id();
         continue;
      }
      else if( !_partial_operations )  // add to the operation history index
         oho = create_oho();

      const operation_history_object& op = *o_op;

      // get the set of accounts this operation applies to
      flat_set<account_uid_type> impacted_uids;
      vector<authority> other;
      operation_get_required_uid_authorities( op.op, impacted_uids, impacted_uids, impacted_uids, other );

      graphene::chain::operation_get_impacted_account_uids( op.op, impacted_uids );

      for( auto& a : other )
         for( auto& item : a.account_uid_auths )
            impacted_uids.insert( item.first.uid );

      // for each operation this account applies to that is in the config link it into the history
      if( _tracked_accounts.size() == 0 )
      {
            // if tracking all accounts, when impacted_uids is not empty (although it will always be),
            //    still need to create oho if _max_ops_per_account > 0 and _partial_operations == true
            //    so always need to create oho if not done
            if (!impacted_uids.empty() && !oho.valid()) { oho = create_oho(); }

            // Note: the check above is for better performance, when the db is not clean,
            //       it breaks consistency of account_stats.total_ops and removed_ops and most_recent_op,
            //       but it ensures it's safe to remove old entries in add_account_history(...)
            for( auto& account_uid : impacted_uids )
            {
                // we don't do index_account_keys here anymore, because
                // that indexing now happens in observers' post_evaluate()

                // add history
                add_account_history( account_uid, oho->id, oho->op.which() );
            }
      }
      else   // tracking a subset of accounts
      {
         // whether need to create oho if _max_ops_per_account > 0 and _partial_operations == true ?
         // the answer: only need to create oho if a tracked account is impacted and need to save history

         if( _max_ops_per_account > 0 )
         {
            // Note: the check above is for better performance, when the db is not clean,
            //       it breaks consistency of account_stats.total_ops and removed_ops and most_recent_op,
            //       but it ensures it's safe to remove old entries in add_account_history(...)
            for( auto account_uid : _tracked_accounts )
            {
               if( impacted_uids.find( account_uid ) != impacted_uids.end() )
               {
                  if (!oho.valid()) { oho = create_oho(); }
                  // add history
                  add_account_history( account_uid, oho->id, oho->op.which() );
               }
            }
         }
      }
      if ( _partial_operations && !oho.valid() )
         skip_oho_id();
   }
}

void account_history_plugin_impl::add_account_history( const account_uid_type account_uid, const operation_history_id_type op_id, uint16_t op_type )
{
   graphene::chain::database& db = database();
   const auto* account_ptr = db.find_account_by_uid( account_uid );
   if( !account_ptr )
      return;
   const auto& account_obj = *account_ptr;
   const auto& stats_obj = account_obj.statistics(db);
   // add new entry
   const auto& ath = db.create<account_transaction_history_object>( [&]( account_transaction_history_object& obj ){
       obj.operation_id = op_id;
       obj.operation_type = op_type;
       obj.account = account_uid;
       obj.sequence = stats_obj.total_ops + 1;
       obj.next = stats_obj.most_recent_op;
   });
   db.modify( stats_obj, [&]( account_statistics_object& obj ){
       obj.most_recent_op = ath.id;
       obj.total_ops = ath.sequence;
   });
   // remove the earliest account history entry if too many
   // _max_ops_per_account is guaranteed to be non-zero outside
   if( stats_obj.total_ops - stats_obj.removed_ops > _max_ops_per_account )
   {
      // look for the earliest entry
      const auto& his_idx = db.get_index_type<account_transaction_history_index>();
      const auto& by_seq_idx = his_idx.indices().get<by_seq>();
      auto itr = by_seq_idx.lower_bound( boost::make_tuple( account_uid, 0 ) );
      // make sure don't remove the one just added
      if( itr != by_seq_idx.end() && itr->account == account_uid && itr->id != ath.id )
      {
         // if found, remove the entry, and adjust account stats object
         const auto remove_op_id = itr->operation_id;
         const auto itr_remove = itr;
         ++itr;
         db.remove( *itr_remove );
         db.modify( stats_obj, [&]( account_statistics_object& obj ){
             obj.removed_ops = obj.removed_ops + 1;
         });
         // modify previous node's next pointer
         // this should be always true, but just have a check here
         if( itr != by_seq_idx.end() && itr->account == account_uid )
         {
            db.modify( *itr, [&]( account_transaction_history_object& obj ){
               obj.next = account_transaction_history_id_type();
            });
         }
         // else need to modify the head pointer, but it shouldn't be true

         // remove the operation history entry if configured and no reference left
         if( _partial_operations )
         {
            // check for references
            const auto& by_opid_idx = his_idx.indices().get<by_opid>();
            if( by_opid_idx.find( remove_op_id ) == by_opid_idx.end() )
            {
               // if no reference, remove
               db.remove( remove_op_id(db) );
            }
         }
      }
   }
}

} // end namespace detail






account_history_plugin::account_history_plugin() :
   my( new detail::account_history_plugin_impl(*this) )
{
}

account_history_plugin::~account_history_plugin()
{
}

std::string account_history_plugin::plugin_name()const
{
   return "account_history";
}

void account_history_plugin::plugin_set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg
   )
{
   cli.add_options()
         ("track-account", boost::program_options::value<string>()->default_value("[]"), "Account ID to track history for (specified as a JSON array)")
         ("partial-operations", boost::program_options::value<bool>(), "Keep only those operations in memory that are related to account history tracking")
         ("max-ops-per-account", boost::program_options::value<uint32_t>(), "Maximum number of operations per account will be kept in memory")
         ;
   cfg.add(cli);
}

void account_history_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   database().applied_block.connect( [&]( const signed_block& b){ my->update_account_histories(b); } );
   my->_oho_index = database().add_index< primary_index< operation_history_index > >();
   database().add_index< primary_index< account_transaction_history_index > >();

   LOAD_VALUE_FLAT_SET(options, "track-account", my->_tracked_accounts, graphene::chain::account_uid_type);
   if (options.count("partial-operations")) {
       my->_partial_operations = options["partial-operations"].as<bool>();
   }
   if (options.count("max-ops-per-account")) {
       my->_max_ops_per_account = options["max-ops-per-account"].as<uint32_t>();
   }
}

void account_history_plugin::plugin_startup()
{
}

flat_set<account_uid_type> account_history_plugin::tracked_accounts() const
{
   return my->_tracked_accounts;
}

} }
